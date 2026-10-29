#pragma once

#include "sujud/PrayTimes.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace sujud::clock {

struct WallTime {
    std::uint16_t year;
    std::uint16_t month;
    std::uint16_t day;
    std::uint16_t hour;
    std::uint16_t minute;
};

// Wall-clock time of an instant in the system's current zone.
WallTime local(Instant instant) noexcept;

// Locale names are BCP-47 ("en-GB"); an empty name means the user's regional format.

// "5:39" with a 12-hour clock, "05:39" with a 24-hour clock.
std::wstring hourMinute(WallTime time, std::wstring const& locale = {});

// "PM" when the locale uses a 12-hour clock.
std::optional<std::wstring> dayPeriod(WallTime time, std::wstring const& locale = {});

// "5:39 PM" or "17:39".
std::wstring shortTime(WallTime time, std::wstring const& locale = {});

// "1:23:45", rounded up so it reaches 0:00:00 exactly on time.
std::wstring countdown(std::chrono::system_clock::time_point now, Instant target);

// "1h 23m", "23m", in the language's own narrow units.
std::wstring narrowDuration(std::chrono::minutes duration, std::wstring const& language);

}
