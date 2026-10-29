#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sujud {

using Instant = std::chrono::sys_seconds;

enum class Method : std::uint8_t { MWL, ISNA, Egypt, Makkah, Karachi, Tehran, Jafari, France, Russia, Singapore };
enum class AsrJuristic : std::uint8_t { Standard, Hanafi };
enum class HighLatitudeRule : std::uint8_t { None, NightMiddle, OneSeventh, AngleBased };

inline constexpr std::array methods{
    Method::MWL, Method::ISNA, Method::Egypt, Method::Makkah, Method::Karachi,
    Method::Tehran, Method::Jafari, Method::France, Method::Russia, Method::Singapore,
};

inline constexpr std::array asrJuristics{ AsrJuristic::Standard, AsrJuristic::Hanafi };

struct Twilight {
    double value;
    bool isMinutes;
};

struct MethodParameters {
    double fajr;
    Twilight isha;
    Twilight maghrib;
    bool jafariMidnight;
};

constexpr MethodParameters parameters(Method method) noexcept {
    constexpr Twilight sunsetPlusOneMinute{ 1, true };
    switch (method) {
    case Method::MWL: return { 18, { 17, false }, sunsetPlusOneMinute, false };
    case Method::ISNA: return { 15, { 15, false }, sunsetPlusOneMinute, false };
    case Method::Egypt: return { 19.5, { 17.5, false }, sunsetPlusOneMinute, false };
    case Method::Makkah: return { 18.5, { 90, true }, sunsetPlusOneMinute, false };
    case Method::Karachi: return { 18, { 18, false }, sunsetPlusOneMinute, false };
    case Method::Tehran: return { 17.7, { 14, false }, { 4.5, false }, true };
    case Method::Jafari: return { 16, { 14, false }, { 4, false }, true };
    case Method::France: return { 12, { 12, false }, sunsetPlusOneMinute, false };
    case Method::Russia: return { 16, { 15, false }, sunsetPlusOneMinute, false };
    case Method::Singapore: return { 20, { 18, false }, sunsetPlusOneMinute, false };
    }
    return parameters(Method::MWL);
}

constexpr double shadowFactor(AsrJuristic asr) noexcept {
    return asr == AsrJuristic::Hanafi ? 2 : 1;
}

std::string_view key(Method method) noexcept;
std::string_view key(AsrJuristic asr) noexcept;
std::optional<Method> methodFromKey(std::string_view key) noexcept;
std::optional<AsrJuristic> asrFromKey(std::string_view key) noexcept;

// A time is empty when the sun never reaches the required position that day.
struct PrayerTimes {
    std::optional<Instant> fajr;
    std::optional<Instant> sunrise;
    std::optional<Instant> dhuhr;
    std::optional<Instant> asr;
    std::optional<Instant> sunset;
    std::optional<Instant> maghrib;
    std::optional<Instant> isha;
    std::optional<Instant> midnight;

    bool operator==(PrayerTimes const&) const = default;
};

struct Calculator {
    Method method = Method::MWL;
    AsrJuristic asr = AsrJuristic::Standard;
    HighLatitudeRule highLatitudes = HighLatitudeRule::NightMiddle;

    PrayerTimes times(std::chrono::year_month_day date, double latitude, double longitude) const noexcept;
};

// Whether the civil date falls in Ramadan on the Umm al-Qura calendar.
bool isRamadan(std::chrono::year_month_day date) noexcept;

}
