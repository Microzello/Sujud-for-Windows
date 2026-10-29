#pragma once

#include "sujud/PrayTimes.h"

#include <array>
#include <chrono>
#include <optional>
#include <string_view>
#include <vector>

namespace sujud {

enum class Prayer : std::uint8_t { Fajr, Sunrise, Dhuhr, Asr, Maghrib, Isha };

inline constexpr std::array prayers{
    Prayer::Fajr, Prayer::Sunrise, Prayer::Dhuhr, Prayer::Asr, Prayer::Maghrib, Prayer::Isha,
};

constexpr bool isPrayer(Prayer prayer) noexcept { return prayer != Prayer::Sunrise; }

std::string_view key(Prayer prayer) noexcept;

struct Event {
    Prayer prayer;
    Instant time;

    bool operator==(Event const&) const = default;
};

struct Schedule {
    PrayerTimes times;

    std::optional<Instant> operator[](Prayer prayer) const noexcept;
    void appendEvents(std::vector<Event>& events) const;
};

struct Status {
    Schedule today;
    std::optional<Event> current;
    std::optional<Event> next;

    double progress(Instant now) const noexcept;
};

struct Coordinates {
    double latitude;
    double longitude;

    bool operator==(Coordinates const&) const = default;
};

// The civil date of an instant in a zone. A null zone is the system's current one.
std::chrono::year_month_day localDate(Instant instant, std::chrono::time_zone const* zone = nullptr) noexcept;

class Timetable {
public:
    explicit Timetable(Coordinates where, Calculator calculator = {}, std::chrono::time_zone const* zone = nullptr) noexcept
        : where(where), calculator(calculator), zone(zone) {}

    Schedule schedule(std::chrono::year_month_day date) const noexcept;
    Schedule schedule(Instant within) const noexcept;
    Status status(Instant at) const;
    std::array<std::optional<Event>, prayers.size()> upcoming(Instant after) const noexcept;
    std::vector<Event> events(Instant startingOn, int days) const;
    std::chrono::year_month_day dateOf(Instant instant) const noexcept { return localDate(instant, zone); }

private:
    Coordinates where;
    Calculator calculator;
    std::chrono::time_zone const* zone;
};

}
