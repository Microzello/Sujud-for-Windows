#include "sujud/Clock.h"

#include <algorithm>
#include <array>
#include <format>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <icu.h>

namespace sujud::clock {
namespace {

constexpr std::wstring_view dayPeriodLetters = L"t";
constexpr std::wstring_view trimmed = L" ‎‏  ";

wchar_t const* localeName(std::wstring const& locale) noexcept {
    return locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.c_str();
}

std::wstring shortTimePattern(std::wstring const& locale) {
    std::array<wchar_t, 80> buffer{};
    if (GetLocaleInfoEx(localeName(locale), LOCALE_SSHORTTIME, buffer.data(), static_cast<int>(buffer.size())) == 0) {
        return L"HH:mm";
    }
    return buffer.data();
}

bool hasDayPeriod(std::wstring_view pattern) noexcept {
    bool quoted = false;
    for (const auto character : pattern) {
        if (character == L'\'') quoted = !quoted;
        if (!quoted && dayPeriodLetters.contains(character)) return true;
    }
    return false;
}

std::wstring withoutDayPeriod(std::wstring_view pattern) {
    std::wstring result;
    bool quoted = false;
    for (const auto character : pattern) {
        if (character == L'\'') quoted = !quoted;
        if (!quoted && dayPeriodLetters.contains(character)) continue;
        result.push_back(character);
    }
    const auto first = result.find_first_not_of(trimmed);
    if (first == std::wstring::npos) return {};
    return result.substr(first, result.find_last_not_of(trimmed) - first + 1);
}

std::wstring format(WallTime time, std::wstring const& locale, wchar_t const* pattern) {
    SYSTEMTIME system{};
    system.wYear = time.year;
    system.wMonth = time.month;
    system.wDay = time.day;
    system.wHour = time.hour;
    system.wMinute = time.minute;
    std::array<wchar_t, 80> buffer{};
    if (GetTimeFormatEx(localeName(locale), 0, &system, pattern, buffer.data(), static_cast<int>(buffer.size())) == 0) {
        return std::format(L"{}:{:02}", time.hour, time.minute);
    }
    return buffer.data();
}

std::string icuLocale(std::wstring const& language) {
    std::string result;
    result.reserve(language.size());
    for (const auto character : language) {
        result.push_back(character == L'-' ? '_' : static_cast<char>(character));
    }
    return result;
}

std::u16string formatUnit(std::u16string_view skeleton, std::int64_t value, char const* locale) {
    UErrorCode status = U_ZERO_ERROR;
    auto* formatter = unumf_openForSkeletonAndLocale(skeleton.data(), static_cast<int32_t>(skeleton.size()), locale, &status);
    auto* result = unumf_openResult(&status);
    std::u16string text;
    if (U_SUCCESS(status)) {
        unumf_formatInt(formatter, value, result, &status);
        std::array<UChar, 64> buffer{};
        const auto length = unumf_resultToString(result, buffer.data(), static_cast<int32_t>(buffer.size()), &status);
        if (U_SUCCESS(status)) {
            text.assign(buffer.data(), static_cast<std::size_t>(length));
        }
    }
    if (result) unumf_closeResult(result);
    if (formatter) unumf_close(formatter);
    return text;
}

std::wstring toWide(std::u16string_view text) {
    return { text.begin(), text.end() };
}

}

WallTime local(Instant instant) noexcept {
    const auto ticks = (instant.time_since_epoch().count() + 11'644'473'600LL) * 10'000'000LL;
    const FILETIME utcFile{ static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32) };
    SYSTEMTIME utc{};
    SYSTEMTIME wall{};
    if (!FileTimeToSystemTime(&utcFile, &utc) || !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &wall)) {
        wall = utc;
    }
    return { wall.wYear, wall.wMonth, wall.wDay, wall.wHour, wall.wMinute };
}

std::wstring hourMinute(WallTime time, std::wstring const& locale) {
    const auto pattern = withoutDayPeriod(shortTimePattern(locale));
    return format(time, locale, pattern.c_str());
}

std::optional<std::wstring> dayPeriod(WallTime time, std::wstring const& locale) {
    if (!hasDayPeriod(shortTimePattern(locale))) {
        return std::nullopt;
    }
    auto text = format(time, locale, L"tt");
    if (text.empty()) {
        return std::nullopt;
    }
    return text;
}

std::wstring shortTime(WallTime time, std::wstring const& locale) {
    return format(time, locale, shortTimePattern(locale).c_str());
}

std::wstring countdown(std::chrono::system_clock::time_point now, Instant target) {
    const auto remaining = std::max(std::chrono::ceil<std::chrono::seconds>(target - now), std::chrono::seconds{ 0 });
    const auto hours = std::chrono::floor<std::chrono::hours>(remaining);
    const auto minutes = std::chrono::floor<std::chrono::minutes>(remaining - hours);
    const auto seconds = remaining - hours - minutes;
    return std::format(L"{}:{:02}:{:02}", hours.count(), minutes.count(), seconds.count());
}

std::wstring narrowDuration(std::chrono::minutes duration, std::wstring const& language) {
    const auto locale = icuLocale(language);
    const auto total = std::max<std::int64_t>(duration.count(), 0);
    const auto hours = total / 60;
    const auto minutes = total % 60;

    const auto minuteText = formatUnit(u"measure-unit/duration-minute unit-width-narrow", minutes, locale.c_str());
    if (hours == 0) {
        return toWide(minuteText);
    }
    const auto hourText = formatUnit(u"measure-unit/duration-hour unit-width-narrow", hours, locale.c_str());
    if (minutes == 0) {
        return toWide(hourText);
    }

    UErrorCode status = U_ZERO_ERROR;
    auto* list = ulistfmt_openForType(locale.c_str(), ULISTFMT_TYPE_UNITS, ULISTFMT_WIDTH_NARROW, &status);
    std::array<UChar const*, 2> items{ hourText.c_str(), minuteText.c_str() };
    std::array<int32_t, 2> lengths{ static_cast<int32_t>(hourText.size()), static_cast<int32_t>(minuteText.size()) };
    std::array<UChar, 128> buffer{};
    int32_t length = 0;
    if (U_SUCCESS(status)) {
        length = ulistfmt_format(list, items.data(), lengths.data(), 2, buffer.data(), static_cast<int32_t>(buffer.size()), &status);
    }
    if (list) ulistfmt_close(list);
    if (U_FAILURE(status)) {
        return toWide(hourText) + L" " + toWide(minuteText);
    }
    return toWide({ buffer.data(), static_cast<std::size_t>(length) });
}

}
