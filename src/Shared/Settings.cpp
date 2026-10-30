#include "pch.h"
#include "Settings.h"

#include <winrt/Windows.Storage.h>

namespace sujud::settings {
namespace {

using winrt::Windows::Foundation::IInspectable;
using winrt::Windows::Foundation::IReference;
using winrt::Windows::Storage::ApplicationData;

constexpr wchar_t methodKey[] = L"calculationMethod";
constexpr wchar_t asrKey[] = L"asrMethod";
constexpr wchar_t latitudeKey[] = L"latitude";
constexpr wchar_t longitudeKey[] = L"longitude";
constexpr wchar_t compassReversedKey[] = L"compassReversed";
constexpr wchar_t fullScreenTimesKey[] = L"fullScreenTimes";

auto values() {
    return ApplicationData::Current().LocalSettings().Values();
}

std::string text(IInspectable const& value) {
    if (const auto string = value.try_as<IReference<winrt::hstring>>()) {
        return winrt::to_string(string.Value());
    }
    return {};
}

std::optional<double> number(IInspectable const& value) {
    if (const auto reference = value.try_as<IReference<double>>()) {
        return reference.Value();
    }
    return std::nullopt;
}

}

Calculator calculator() {
    const auto stored = values();
    Calculator result;
    result.method = methodFromKey(text(stored.TryLookup(methodKey))).value_or(Method::MWL);
    result.asr = asrFromKey(text(stored.TryLookup(asrKey))).value_or(AsrJuristic::Standard);
    return result;
}

void setMethod(Method method) {
    values().Insert(methodKey, winrt::box_value(winrt::to_hstring(key(method))));
}

void setAsr(AsrJuristic asr) {
    values().Insert(asrKey, winrt::box_value(winrt::to_hstring(key(asr))));
}

std::optional<Coordinates> location() {
    const auto stored = values();
    const auto latitude = number(stored.TryLookup(latitudeKey));
    const auto longitude = number(stored.TryLookup(longitudeKey));
    if (!latitude || !longitude) {
        return std::nullopt;
    }
    return Coordinates{ *latitude, *longitude };
}

void setLocation(Coordinates where) {
    if (!(where.latitude >= -90 && where.latitude <= 90 && where.longitude >= -180 && where.longitude <= 180)) {
        return;
    }
    const auto stored = values();
    stored.Insert(latitudeKey, winrt::box_value(where.latitude));
    stored.Insert(longitudeKey, winrt::box_value(where.longitude));
}

std::optional<Timetable> timetable() {
    if (const auto where = location()) {
        return Timetable{ *where, calculator() };
    }
    return std::nullopt;
}

bool compassReversed() {
    return winrt::unbox_value_or<bool>(values().TryLookup(compassReversedKey), false);
}

void setCompassReversed(bool reversed) {
    values().Insert(compassReversedKey, winrt::box_value(reversed));
}

bool fullScreenTimes() {
    return winrt::unbox_value_or<bool>(values().TryLookup(fullScreenTimesKey), false);
}

void setFullScreenTimes(bool show) {
    values().Insert(fullScreenTimesKey, winrt::box_value(show));
}

}
