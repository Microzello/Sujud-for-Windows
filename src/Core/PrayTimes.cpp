// Native rewrite of praytime.js v3.2 by Hamid Zarrabi-Zadeh (https://praytimes.org, MIT).
// Every expression keeps the reference's operation order so results match it to the minute.

#include "sujud/PrayTimes.h"

#include <algorithm>
#include <cmath>
#include <icu.h>
#include <iterator>
#include <numbers>
#include <vector>

namespace sujud {
namespace {

constexpr double pi = std::numbers::pi;
constexpr double horizon = 0.833;

double dtr(double degrees) noexcept { return degrees * pi / 180; }
double rtd(double radians) noexcept { return radians * 180 / pi; }
double dsin(double degrees) noexcept { return std::sin(dtr(degrees)); }
double dcos(double degrees) noexcept { return std::cos(dtr(degrees)); }
double dtan(double degrees) noexcept { return std::tan(dtr(degrees)); }
double darcsin(double x) noexcept { return rtd(std::asin(x)); }
double darccos(double x) noexcept { return rtd(std::acos(x)); }
double darccot(double x) noexcept { return rtd(std::atan(1 / x)); }
double darctan2(double y, double x) noexcept { return rtd(std::atan2(y, x)); }
// Same result as fmod(fmod(a, b) + b, b); the outer fmod is exact subtraction by Sterbenz's lemma.
double mod(double a, double b) noexcept {
    double x = std::fmod(a, b) + b;
    if (x >= b) x -= b;
    return x >= b ? x - b : x;
}

struct SunPosition {
    double declination;
    double equation;
};

// Times are hours of local solar time on the given UTC day.
class Engine {
public:
    Engine(double utcMilliseconds, double latitude, double longitude) noexcept
        : utcMilliseconds(utcMilliseconds), latitude(latitude), longitude(longitude),
          sinLatitude(dsin(latitude)), cosLatitude(dcos(latitude)) {}

    SunPosition sun(double time) const noexcept {
        const double d = utcMilliseconds / 864e5 - 10957.5 + time / 24 - longitude / 360;

        const double g = mod(357.529 + 0.98560028 * d, 360);
        const double q = mod(280.459 + 0.98564736 * d, 360);
        const double l = mod(q + 1.915 * dsin(g) + 0.020 * dsin(2 * g), 360);
        const double e = 23.439 - 0.00000036 * d;
        const double sinL = dsin(l);
        const double ra = mod(darctan2(dcos(e) * sinL, dcos(l)) / 15, 24);

        return { darcsin(dsin(e) * sinL), q / 15 - ra };
    }

    static double midDay(SunPosition const& sun) noexcept {
        return mod(12 - sun.equation, 24);
    }

    double angleTime(double angle, SunPosition const& sun, double direction) const noexcept {
        const double numerator = -dsin(angle) - sinLatitude * dsin(sun.declination);
        const double diff = darccos(numerator / (cosLatitude * dcos(sun.declination))) / 15;
        return midDay(sun) + diff * direction;
    }

    double asrAngle(double factor, SunPosition const& sun) const noexcept {
        return -darccot(factor + dtan(std::abs(latitude - sun.declination)));
    }

    std::optional<Instant> convert(double time) const noexcept {
        const double timestamp = utcMilliseconds + std::floor((time - longitude / 15) * 36e5);
        const double rounded = std::floor(timestamp / 6e4 + 0.5) * 6e4;
        if (!std::isfinite(rounded)) {
            return std::nullopt;
        }
        return Instant{ std::chrono::seconds{ static_cast<std::int64_t>(rounded / 1000) } };
    }

private:
    double utcMilliseconds;
    double latitude;
    double longitude;
    double sinLatitude;
    double cosLatitude;
};

double highLatitudeFactor(HighLatitudeRule rule, double angle) noexcept {
    switch (rule) {
    case HighLatitudeRule::NightMiddle: return 1.0 / 2;
    case HighLatitudeRule::OneSeventh: return 1.0 / 7;
    case HighLatitudeRule::AngleBased: return 1.0 / 60 * angle;
    case HighLatitudeRule::None: break;
    }
    return std::nan("");
}

struct HighLatitudeAdjuster {
    HighLatitudeRule rule;
    double night;
    bool adjusted = false;

    double operator()(double time, double base, double angle, double direction) noexcept {
        const double portion = highLatitudeFactor(rule, angle) * night;
        const double timeDiff = (time - base) * direction;
        if (std::isnan(time) || timeDiff > portion) {
            adjusted = true;
            return base + portion * direction;
        }
        return time;
    }
};

constexpr std::array<std::pair<Method, std::string_view>, 10> methodKeys{ {
    { Method::MWL, "MWL" },
    { Method::ISNA, "ISNA" },
    { Method::Egypt, "Egypt" },
    { Method::Makkah, "Makkah" },
    { Method::Karachi, "Karachi" },
    { Method::Tehran, "Tehran" },
    { Method::Jafari, "Jafari" },
    { Method::France, "France" },
    { Method::Russia, "Russia" },
    { Method::Singapore, "Singapore" },
} };

}

std::string_view key(Method method) noexcept {
    return methodKeys[static_cast<std::size_t>(method)].second;
}

std::string_view key(AsrJuristic asr) noexcept {
    return asr == AsrJuristic::Hanafi ? "Hanafi" : "Standard";
}

std::optional<Method> methodFromKey(std::string_view value) noexcept {
    for (auto const& [method, name] : methodKeys) {
        if (name == value) {
            return method;
        }
    }
    return std::nullopt;
}

std::optional<AsrJuristic> asrFromKey(std::string_view value) noexcept {
    if (value == "Standard") return AsrJuristic::Standard;
    if (value == "Hanafi") return AsrJuristic::Hanafi;
    return std::nullopt;
}

PrayerTimes Calculator::times(std::chrono::year_month_day date, double latitude, double longitude) const noexcept {
    auto params = parameters(method);
    if (method == Method::Makkah && isRamadan(date)) {
        params.isha = { 120, true };
    }

    const auto days = std::chrono::sys_days{ date }.time_since_epoch().count();
    const Engine engine{ static_cast<double>(days) * 864e5, latitude, longitude };

    const auto sun5 = engine.sun(5);
    const auto sun6 = engine.sun(6);
    const auto sun12 = engine.sun(12);
    const auto sun13 = engine.sun(13);
    const auto sun18 = engine.sun(18);
    const auto sun24 = engine.sun(24);

    double fajr = engine.angleTime(params.fajr, sun5, -1);
    const double sunrise = engine.angleTime(horizon, sun6, -1);
    const double dhuhr = Engine::midDay(sun12);
    const double afternoon = engine.angleTime(engine.asrAngle(shadowFactor(asr), sun13), sun13, 1);
    const double sunset = engine.angleTime(horizon, sun18, 1);
    double maghrib = params.maghrib.isMinutes ? 0 : engine.angleTime(params.maghrib.value, sun18, 1);
    double isha = params.isha.isMinutes ? 0 : engine.angleTime(params.isha.value, sun18, 1);
    double midnight = Engine::midDay(sun24) + 12;

    HighLatitudeAdjuster adjust{ highLatitudes, 24 + sunrise - sunset };
    if (highLatitudes != HighLatitudeRule::None) {
        fajr = adjust(fajr, sunrise, params.fajr, -1);
        if (!params.isha.isMinutes) {
            isha = adjust(isha, sunset, params.isha.value, 1);
        }
        if (!params.maghrib.isMinutes) {
            maghrib = adjust(maghrib, sunset, params.maghrib.value, 1);
        }
    }

    if (params.maghrib.isMinutes) {
        maghrib = sunset + params.maghrib.value / 60;
    }
    if (params.isha.isMinutes) {
        isha = maghrib + params.isha.value / 60;
    }
    if (params.jafariMidnight) {
        const double nextFajr = engine.angleTime(params.fajr, engine.sun(29), -1) + 24;
        midnight = (sunset + (adjust.adjusted ? fajr + 24 : nextFajr)) / 2;
    }

    return {
        engine.convert(fajr),
        engine.convert(sunrise),
        engine.convert(dhuhr),
        engine.convert(afternoon),
        engine.convert(sunset),
        engine.convert(maghrib),
        engine.convert(isha),
        engine.convert(midnight),
    };
}

namespace {

// One Hijri year counted from 1 Ramadan, in days since 1970.
struct RamadanCycle {
    int start;
    int end;
    int nextStart;
};

class UmmAlQura {
public:
    UmmAlQura() noexcept {
        UErrorCode status = U_ZERO_ERROR;
        calendar = ucal_open(u"UTC", -1, "@calendar=islamic-umalqura", UCAL_DEFAULT, &status);
        if (U_FAILURE(status)) {
            calendar = nullptr;
        }
    }

    ~UmmAlQura() {
        if (calendar) {
            ucal_close(calendar);
        }
    }

    UmmAlQura(UmmAlQura const&) = delete;
    UmmAlQura& operator=(UmmAlQura const&) = delete;

    bool isRamadan(int day) {
        const auto cached = std::ranges::upper_bound(cycles, day, {}, &RamadanCycle::start);
        if (cached != cycles.begin() && day < std::prev(cached)->nextStart) {
            return day < std::prev(cached)->end;
        }
        const auto cycle = cycleContaining(day);
        if (!cycle) {
            return false;
        }
        cycles.insert(std::ranges::upper_bound(cycles, cycle->start, {}, &RamadanCycle::start), *cycle);
        return day < cycle->end;
    }

private:
    // UCAL_MONTH is zero-based, so Ramadan is 8 and Shawwal 9.
    static constexpr int ramadan = 8;

    std::optional<RamadanCycle> cycleContaining(int day) {
        if (!calendar) {
            return std::nullopt;
        }
        UErrorCode status = U_ZERO_ERROR;
        ucal_setMillis(calendar, (day + 0.5) * 864e5, &status);
        const auto year = ucal_get(calendar, UCAL_YEAR, &status);
        const auto month = ucal_get(calendar, UCAL_MONTH, &status);
        const auto cycleYear = month < ramadan ? year - 1 : year;
        RamadanCycle cycle{
            firstDay(cycleYear, ramadan, status),
            firstDay(cycleYear, ramadan + 1, status),
            firstDay(cycleYear + 1, ramadan, status),
        };
        if (U_FAILURE(status)) {
            return std::nullopt;
        }
        return cycle;
    }

    int firstDay(int year, int month, UErrorCode& status) {
        ucal_clear(calendar);
        ucal_setDate(calendar, year, month, 1, &status);
        return static_cast<int>(std::floor(ucal_getMillis(calendar, &status) / 864e5));
    }

    UCalendar* calendar = nullptr;
    std::vector<RamadanCycle> cycles;
};

}

bool isRamadan(std::chrono::year_month_day date) noexcept {
    thread_local UmmAlQura calendar;
    try {
        return calendar.isRamadan(static_cast<int>(std::chrono::sys_days{ date }.time_since_epoch().count()));
    } catch (...) {
        return false;
    }
}

}
