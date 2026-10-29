#include "sujud/Timetable.h"

#include <algorithm>
#include <ranges>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace sujud {
namespace {

using std::chrono::days;
using std::chrono::sys_days;
using std::chrono::year_month_day;

year_month_day addDays(year_month_day date, int count) noexcept {
    return year_month_day{ sys_days{ date } + days{ count } };
}

}

std::string_view key(Prayer prayer) noexcept {
    switch (prayer) {
    case Prayer::Fajr: return "fajr";
    case Prayer::Sunrise: return "sunrise";
    case Prayer::Dhuhr: return "dhuhr";
    case Prayer::Asr: return "asr";
    case Prayer::Maghrib: return "maghrib";
    case Prayer::Isha: return "isha";
    }
    return {};
}

std::optional<Instant> Schedule::operator[](Prayer prayer) const noexcept {
    switch (prayer) {
    case Prayer::Fajr: return times.fajr;
    case Prayer::Sunrise: return times.sunrise;
    case Prayer::Dhuhr: return times.dhuhr;
    case Prayer::Asr: return times.asr;
    case Prayer::Maghrib: return times.maghrib;
    case Prayer::Isha: return times.isha;
    }
    return std::nullopt;
}

void Schedule::appendEvents(std::vector<Event>& events) const {
    for (const auto prayer : prayers) {
        if (const auto time = (*this)[prayer]) {
            events.push_back({ prayer, *time });
        }
    }
}

double Status::progress(Instant now) const noexcept {
    if (!current || !next) {
        return 0;
    }
    const auto total = (next->time - current->time).count();
    if (total <= 0) {
        return 0;
    }
    return std::clamp(static_cast<double>((now - current->time).count()) / static_cast<double>(total), 0.0, 1.0);
}

year_month_day localDate(Instant instant, std::chrono::time_zone const* zone) noexcept {
    if (zone) {
        return year_month_day{ std::chrono::floor<days>(zone->to_local(instant)) };
    }

    const auto ticks = (instant.time_since_epoch().count() + 11'644'473'600LL) * 10'000'000LL;
    const FILETIME utcFile{ static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32) };
    SYSTEMTIME utc{};
    SYSTEMTIME local{};
    if (!FileTimeToSystemTime(&utcFile, &utc) || !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) {
        return year_month_day{ std::chrono::floor<days>(instant) };
    }
    return std::chrono::year{ local.wYear } / local.wMonth / local.wDay;
}

Schedule Timetable::schedule(year_month_day date) const noexcept {
    return { calculator.times(date, where.latitude, where.longitude) };
}

Schedule Timetable::schedule(Instant within) const noexcept {
    return schedule(dateOf(within));
}

Status Timetable::status(Instant at) const {
    const auto date = dateOf(at);
    const auto today = schedule(date);

    std::vector<Event> events;
    events.reserve(prayers.size() * 3);
    schedule(addDays(date, -1)).appendEvents(events);
    today.appendEvents(events);
    schedule(addDays(date, 1)).appendEvents(events);

    Status status{ today };
    const auto last = std::ranges::find_last_if(events, [at](Event const& event) { return event.time <= at; });
    if (!last.empty()) {
        status.current = last.front();
    }
    const auto first = std::ranges::find_if(events, [at](Event const& event) { return event.time > at; });
    if (first != events.end()) {
        status.next = *first;
    }
    return status;
}

std::array<std::optional<Event>, prayers.size()> Timetable::upcoming(Instant after) const noexcept {
    const auto date = dateOf(after);
    const auto today = schedule(date);
    const auto tomorrow = schedule(addDays(date, 1));

    std::array<std::optional<Event>, prayers.size()> result;
    for (const auto prayer : prayers) {
        auto& slot = result[static_cast<std::size_t>(prayer)];
        if (const auto time = today[prayer]; time && *time > after) {
            slot = Event{ prayer, *time };
        } else if (const auto next = tomorrow[prayer]) {
            slot = Event{ prayer, *next };
        }
    }
    return result;
}

std::vector<Event> Timetable::events(Instant startingOn, int count) const {
    const auto first = dateOf(startingOn);
    std::vector<Event> result;
    result.reserve(prayers.size() * static_cast<std::size_t>(std::max(count, 0)));
    for (int offset = 0; offset < count; ++offset) {
        schedule(addDays(first, offset)).appendEvents(result);
    }
    return result;
}

}
