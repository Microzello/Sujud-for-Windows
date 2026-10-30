#include "Tools.h"

#include "sujud/Qibla.h"
#include "sujud/Timetable.h"

#include <array>
#include <charconv>
#include <cmath>
#include <format>
#include <numbers>
#include <string>

namespace sujud::mcp {
namespace {

using namespace std::chrono;
using namespace winrt::Windows::Data::Json;

JsonValue string(std::string_view value) {
    return JsonValue::CreateStringValue(winrt::to_hstring(value));
}

std::string utf8(wchar_t const* value) {
    return winrt::to_string(value);
}

double number(JsonObject const& arguments, wchar_t const* name, double low, double high) {
    if (!arguments.HasKey(name) || arguments.GetNamedValue(name).ValueType() != JsonValueType::Number) {
        throw InvalidArguments(std::format("{} is required and must be a number.", utf8(name)));
    }
    const double value = arguments.GetNamedNumber(name);
    if (!std::isfinite(value) || value < low || value > high) {
        throw InvalidArguments(std::format("{} must be between {} and {}.", utf8(name), low, high));
    }
    return value;
}

std::optional<std::string> text(JsonObject const& arguments, wchar_t const* name) {
    if (!arguments.HasKey(name)) {
        return std::nullopt;
    }
    const auto value = arguments.GetNamedValue(name);
    if (value.ValueType() == JsonValueType::Null) {
        return std::nullopt;
    }
    if (value.ValueType() != JsonValueType::String) {
        throw InvalidArguments(std::format("{} must be a string.", utf8(name)));
    }
    return winrt::to_string(value.GetString());
}

Coordinates coordinates(JsonObject const& arguments) {
    return { number(arguments, L"latitude", -90, 90), number(arguments, L"longitude", -180, 180) };
}

time_zone const* zone(JsonObject const& arguments) {
    const auto name = text(arguments, L"time_zone");
    if (!name) {
        return current_zone();
    }
    try {
        return locate_zone(*name);
    } catch (std::runtime_error const&) {
        throw InvalidArguments(std::format("{} isn't a known time zone. Use an IANA name such as Europe/London.", *name));
    }
}

Calculator calculator(JsonObject const& arguments) {
    Calculator result;
    if (const auto name = text(arguments, L"method")) {
        const auto method = methodFromKey(*name);
        if (!method) {
            throw InvalidArguments(std::format("{} isn't a calculation method.", *name));
        }
        result.method = *method;
    }
    if (const auto name = text(arguments, L"asr")) {
        const auto asr = asrFromKey(*name);
        if (!asr) {
            throw InvalidArguments("asr must be Standard or Hanafi.");
        }
        result.asr = *asr;
    }
    return result;
}

year_month_day date(JsonObject const& arguments, time_zone const* where, system_clock::time_point now) {
    const auto value = text(arguments, L"date");
    if (!value) {
        return localDate(floor<seconds>(now), where);
    }
    int y = 0;
    unsigned m = 0;
    unsigned d = 0;
    const auto field = [&value](std::size_t offset, std::size_t length, auto& out) {
        const auto first = value->data() + offset;
        const auto [end, error] = std::from_chars(first, first + length, out);
        return error == std::errc{} && end == first + length;
    };
    const bool shaped = value->size() == 10 && (*value)[4] == '-' && (*value)[7] == '-';
    if (!shaped || !field(0, 4, y) || !field(5, 2, m) || !field(8, 2, d)) {
        throw InvalidArguments("date must be YYYY-MM-DD.");
    }
    const year_month_day parsed{ year{ y }, month{ m }, day{ d } };
    if (!parsed.ok()) {
        throw InvalidArguments(std::format("{} isn't a real date.", *value));
    }
    return parsed;
}

std::string isoTime(Instant instant, time_zone const* where) {
    const auto offset = where->get_info(instant).offset;
    const auto local = local_seconds{ instant.time_since_epoch() + offset };
    const auto minutesEast = duration_cast<minutes>(offset).count();
    const auto magnitude = minutesEast < 0 ? -minutesEast : minutesEast;
    return std::format("{:%FT%T}{}{:02}:{:02}", local, minutesEast < 0 ? '-' : '+', magnitude / 60, magnitude % 60);
}

JsonValue optionalTime(std::optional<Instant> instant, time_zone const* where) {
    return instant ? string(isoTime(*instant, where)) : JsonValue::CreateNullValue();
}

JsonValue optionalPrayer(std::optional<Event> event) {
    return event ? string(key(event->prayer)) : JsonValue::CreateNullValue();
}

JsonObject prayerTimes(JsonObject const& arguments, system_clock::time_point now) {
    const auto where = coordinates(arguments);
    const auto zoneOfPlace = zone(arguments);
    const auto method = calculator(arguments);
    const auto day = date(arguments, zoneOfPlace, now);
    const auto schedule = Timetable{ where, method, zoneOfPlace }.schedule(day);

    JsonObject times;
    for (const auto prayer : prayers) {
        times.SetNamedValue(winrt::to_hstring(key(prayer)), optionalTime(schedule[prayer], zoneOfPlace));
    }

    JsonObject result;
    result.SetNamedValue(L"date", string(std::format("{:%F}", sys_days{ day })));
    result.SetNamedValue(L"time_zone", string(zoneOfPlace->name()));
    result.SetNamedValue(L"method", string(key(method.method)));
    result.SetNamedValue(L"asr", string(key(method.asr)));
    result.SetNamedValue(L"times", times);
    return result;
}

JsonObject nextPrayer(JsonObject const& arguments, system_clock::time_point now) {
    const auto where = coordinates(arguments);
    const auto zoneOfPlace = zone(arguments);
    const auto at = floor<seconds>(now);
    const auto status = Timetable{ where, calculator(arguments), zoneOfPlace }.status(at);

    JsonObject result;
    result.SetNamedValue(L"prayer", optionalPrayer(status.next));
    result.SetNamedValue(L"time", optionalTime(status.next ? std::optional{ status.next->time } : std::nullopt, zoneOfPlace));
    result.SetNamedValue(L"minutes_until", status.next
        ? JsonValue::CreateNumberValue(static_cast<double>(ceil<minutes>(status.next->time - at).count()))
        : JsonValue::CreateNullValue());
    result.SetNamedValue(L"current", optionalPrayer(status.current));
    result.SetNamedValue(L"time_zone", string(zoneOfPlace->name()));
    return result;
}

JsonObject qiblaDirection(JsonObject const& arguments) {
    const auto where = coordinates(arguments);
    const double bearing = qibla::bearing(where.latitude, where.longitude);

    constexpr std::array<std::string_view, 16> points{
        "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW",
    };
    const auto point = points[static_cast<std::size_t>((bearing + 11.25) / 22.5) % points.size()];

    constexpr double toRadians = std::numbers::pi / 180;
    constexpr double earthRadiusKm = 6371.0088;
    const double dPhi = (qibla::kaabaLatitude - where.latitude) * toRadians;
    const double dLambda = (qibla::kaabaLongitude - where.longitude) * toRadians;
    const double h = std::pow(std::sin(dPhi / 2), 2)
        + std::cos(where.latitude * toRadians) * std::cos(qibla::kaabaLatitude * toRadians) * std::pow(std::sin(dLambda / 2), 2);
    const double distance = 2 * earthRadiusKm * std::asin(std::min(1.0, std::sqrt(h)));

    JsonObject result;
    result.SetNamedValue(L"bearing_degrees", JsonValue::CreateNumberValue(std::round(bearing * 10) / 10));
    result.SetNamedValue(L"compass_point", string(point));
    result.SetNamedValue(L"distance_km", JsonValue::CreateNumberValue(std::round(distance)));
    return result;
}

}

std::optional<JsonObject> call(std::wstring_view tool, JsonObject const& arguments, system_clock::time_point now) {
    if (tool == L"get_prayer_times") {
        return prayerTimes(arguments, now);
    }
    if (tool == L"get_next_prayer") {
        return nextPrayer(arguments, now);
    }
    if (tool == L"get_qibla_direction") {
        return qiblaDirection(arguments);
    }
    return std::nullopt;
}

}
