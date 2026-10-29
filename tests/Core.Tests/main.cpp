#include "sujud/Clock.h"
#include "sujud/PrayTimes.h"
#include "sujud/Qibla.h"
#include "sujud/Timetable.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <icu.h>
#include <limits>
#include <print>
#include <source_location>
#include <string_view>

using namespace std::chrono;
using namespace sujud;

namespace {

int failures = 0;

void check(bool condition, std::string_view what, std::source_location where = std::source_location::current()) {
    if (!condition) {
        ++failures;
        std::println("FAIL {}:{} {}", where.file_name(), where.line(), what);
    }
}

constexpr std::int64_t none = std::numeric_limits<std::int64_t>::min();

struct ReferenceCase {
    std::string_view place;
    double latitude;
    double longitude;
    Method method;
    AsrJuristic asr;
    HighLatitudeRule highLatitudes;
    int year;
    unsigned month;
    unsigned day;
    std::array<std::int64_t, 8> minutes;
};

using M = Method;
using A = AsrJuristic;
using H = HighLatitudeRule;

constexpr ReferenceCase referenceCases[] = {
#include "ReferenceCases.inc"
};

std::int64_t minutesOf(std::optional<Instant> time) {
    return time ? floor<std::chrono::minutes>(*time).time_since_epoch().count() : none;
}

std::array<std::int64_t, 8> minutesOf(PrayerTimes const& t) {
    return { minutesOf(t.fajr), minutesOf(t.sunrise), minutesOf(t.dhuhr), minutesOf(t.asr),
             minutesOf(t.sunset), minutesOf(t.maghrib), minutesOf(t.isha), minutesOf(t.midnight) };
}

year_month_day date(int y, unsigned m, unsigned d) {
    return year{ y } / month{ m } / day{ d };
}

void matchesPraytimeJs() {
    int mismatches = 0;
    for (auto const& reference : referenceCases) {
        const Calculator calculator{ reference.method, reference.asr, reference.highLatitudes };
        const auto actual = minutesOf(calculator.times(date(reference.year, reference.month, reference.day), reference.latitude, reference.longitude));
        if (actual != reference.minutes) {
            ++mismatches;
            std::println("  {} {} {}-{}-{} method {} asr {} high {}", reference.place, reference.latitude, reference.year, reference.month,
                         reference.day, key(reference.method), key(reference.asr), static_cast<int>(reference.highLatitudes));
            for (std::size_t i = 0; i < actual.size(); ++i) {
                if (actual[i] != reference.minutes[i]) {
                    std::println("    time {} expected {} got {}", i, reference.minutes[i], actual[i]);
                }
            }
        }
    }
    check(mismatches == 0, std::format("{} of {} praytime.js cases differ", mismatches, std::size(referenceCases)));
    std::println("{} praytime.js reference cases", std::size(referenceCases));
}

void matchesIosApp() {
    struct IosCase {
        double latitude;
        double longitude;
        Method method;
        AsrJuristic asr;
        year_month_day date;
        std::array<std::int64_t, 7> milliseconds;
    };
    constexpr std::int64_t nil = -1;
    const IosCase cases[] = {
        { 43.65, -79.38, Method::MWL, AsrJuristic::Standard, date(2026, 6, 15),
          { 1781507580000, 1781516100000, 1781543880000, 1781558640000, 1781571660000, 1781571720000, 1781579520000 } },
        { 51.5074, -0.1278, Method::ISNA, AsrJuristic::Hanafi, date(2026, 12, 15),
          { 1797315360000, 1797321600000, 1797335760000, 1797343500000, 1797349920000, 1797349980000, 1797356100000 } },
        { 21.4225, 39.8262, Method::Makkah, AsrJuristic::Standard, date(2026, 4, 15),
          { 1776217380000, 1776222120000, 1776244860000, 1776257100000, 1776267600000, 1776267660000, 1776273060000 } },
        { 35.69, 51.39, Method::Tehran, AsrJuristic::Standard, date(2026, 9, 28),
          { 1790557380000, 1790562420000, 1790583900000, 1790596140000, 1790605380000, 1790606460000, 1790609280000 } },
        { 69.65, 18.96, Method::MWL, AsrJuristic::Standard, date(2026, 6, 15),
          { nil, nil, 1781520300000, 1781538960000, nil, nil, nil } },
    };
    for (auto const& c : cases) {
        const auto t = Calculator{ c.method, c.asr }.times(c.date, c.latitude, c.longitude);
        const std::optional<Instant> actual[] = { t.fajr, t.sunrise, t.dhuhr, t.asr, t.sunset, t.maghrib, t.isha };
        for (std::size_t i = 0; i < c.milliseconds.size(); ++i) {
            const auto ms = actual[i] ? actual[i]->time_since_epoch().count() * 1000 : nil;
            check(ms == c.milliseconds[i], std::format("iOS case {} time {}", key(c.method), i));
        }
    }
}

bool isRamadanByIcu(year_month_day date) {
    UErrorCode status = U_ZERO_ERROR;
    auto* calendar = ucal_open(u"UTC", -1, "@calendar=islamic-umalqura", UCAL_DEFAULT, &status);
    ucal_setMillis(calendar, (sys_days{ date }.time_since_epoch().count() + 0.5) * 864e5, &status);
    const auto month = ucal_get(calendar, UCAL_MONTH, &status);
    ucal_close(calendar);
    return U_SUCCESS(status) && month == 8;
}

void ramadanFollowsUmmAlQura() {
    check(!isRamadan(date(2026, 2, 17)), "17 February 2026 is 29 Sha'ban");
    check(isRamadan(date(2026, 2, 18)), "18 February 2026 is 1 Ramadan");
    check(isRamadan(date(2026, 3, 19)), "19 March 2026 is 30 Ramadan");
    check(!isRamadan(date(2026, 3, 20)), "20 March 2026 is 1 Shawwal");
    check(isRamadan(date(2027, 3, 8)) && !isRamadan(date(2027, 3, 9)), "Ramadan 1448 has 29 days");

    int mismatches = 0;
    for (auto day = sys_days{ date(2060, 12, 31) }; day >= sys_days{ date(1990, 1, 1) }; day -= std::chrono::days{ 1 }) {
        mismatches += isRamadan(year_month_day{ day }) != isRamadanByIcu(year_month_day{ day });
    }
    check(mismatches == 0, std::format("{} days disagree with ICU", mismatches));
}

void ummAlQuraIshaIsLaterDuringRamadan() {
    check(isRamadan(date(2026, 3, 1)), "1 March 2026 is in Ramadan 1447");
    check(!isRamadan(date(2026, 4, 15)), "15 April 2026 is not in Ramadan");
    const auto t = Calculator{ Method::Makkah }.times(date(2026, 3, 1), 21.4225, 39.8262);
    check(t.maghrib && t.isha && *t.isha - *t.maghrib == 120min, "Isha is 120 minutes after Maghrib in Ramadan");
}

void keysRoundTrip() {
    for (const auto method : methods) {
        check(methodFromKey(key(method)) == method, "method key round-trips");
    }
    for (const auto asr : asrJuristics) {
        check(asrFromKey(key(asr)) == asr, "asr key round-trips");
    }
    check(!methodFromKey("Nope"), "unknown method key");
}

struct TorontoTimetable {
    time_zone const* zone = locate_zone("America/Toronto");
    Timetable timetable{ { 43.65, -79.38 }, {}, zone };

    Instant at(int hour, int minute, unsigned dayOfMonth = 15) const {
        const local_days localDay{ year{ 2026 } / June / day{ dayOfMonth } };
        return floor<seconds>(zone->to_sys(localDay + hours{ hour } + minutes{ minute }));
    }

    unsigned dayOf(Instant instant) const {
        return static_cast<unsigned>(localDate(instant, zone).day());
    }
};

void timetableStatus() {
    const TorontoTimetable toronto;

    const auto afternoon = toronto.timetable.status(toronto.at(18, 0));
    check(afternoon.current && afternoon.current->prayer == Prayer::Asr, "6 PM is Asr");
    check(afternoon.next && afternoon.next->prayer == Prayer::Maghrib, "Maghrib is next at 6 PM");

    const auto beforeFajr = toronto.timetable.status(toronto.at(1, 0));
    check(beforeFajr.current && beforeFajr.current->prayer == Prayer::Isha, "1 AM is still Isha");
    check(beforeFajr.next && beforeFajr.next->prayer == Prayer::Fajr && toronto.dayOf(beforeFajr.next->time) == 15, "Fajr is next, same day");

    const auto lateNight = toronto.timetable.status(toronto.at(23, 30));
    check(lateNight.current && lateNight.current->prayer == Prayer::Isha, "11:30 PM is Isha");
    check(lateNight.next && lateNight.next->prayer == Prayer::Fajr && toronto.dayOf(lateNight.next->time) == 16, "tomorrow's Fajr is next");

    const auto now = toronto.at(18, 0);
    const auto progress = toronto.timetable.status(now).progress(now);
    check(progress > 0 && progress < 1, "progress is between 0 and 1");
}

void timetableUpcoming() {
    const TorontoTimetable toronto;
    const auto upcoming = toronto.timetable.upcoming(toronto.at(18, 0));
    for (const auto prayer : prayers) {
        auto const& event = upcoming[static_cast<std::size_t>(prayer)];
        check(event.has_value(), "every prayer comes around");
        if (!event) continue;
        const bool passed = prayer == Prayer::Fajr || prayer == Prayer::Sunrise || prayer == Prayer::Dhuhr || prayer == Prayer::Asr;
        check(toronto.dayOf(event->time) == (passed ? 16u : 15u), std::format("{} is on the right day", key(prayer)));
    }
}

void timetableEvents() {
    const TorontoTimetable toronto;
    const auto events = toronto.timetable.events(toronto.at(12, 0), 3);
    check(events.size() == 18, "three days of events");
    check(std::ranges::is_sorted(events, {}, &Event::time), "events are in order");
}

void qiblaBearings() {
    check(std::abs(qibla::bearing(40.7128, -74.0060) - 58.5) < 0.5, "New York faces 58.5 degrees");
    check(std::abs(qibla::bearing(51.5074, -0.1278) - 119.0) < 0.5, "London faces 119 degrees");
    check(qibla::angleDifference(350, 10) == 20, "350 to 10 is +20");
    check(qibla::angleDifference(10, 350) == -20, "10 to 350 is -20");
    check(qibla::angleDifference(90, 90) == 0, "no difference");
}

void timeFormatting() {
    const clock::WallTime morning{ 2026, 9, 30, 5, 39 };
    check(clock::hourMinute(morning, L"en-US") == L"5:39", "12-hour time drops AM/PM without padding");
    check(clock::dayPeriod(morning, L"en-US") == L"AM", "12-hour time has a day period");
    check(clock::hourMinute(morning, L"en-GB") == L"05:39", "24-hour time");
    check(!clock::dayPeriod(morning, L"en-GB"), "24-hour time has no day period");

    const auto target = Instant{ seconds{ 1'800'000'000 } };
    check(clock::countdown(target - 5025s, target) == L"1:23:45", "countdown");
    check(clock::countdown(target - 1500ms, target) == L"0:00:02", "countdown rounds up");
    check(clock::countdown(target + 3s, target) == L"0:00:00", "countdown stops at zero");

    check(clock::narrowDuration(83min, L"en-US") == L"1h 23m", "narrow hours and minutes");
    check(clock::narrowDuration(7min, L"en-US") == L"7m", "narrow minutes");
    check(clock::narrowDuration(120min, L"en-US") == L"2h", "narrow hours");
}

int benchmark() {
    const Calculator calculators[] = { { Method::MWL }, { Method::Makkah }, { Method::Tehran } };
    constexpr int days = 365 * 100;
    const year_month_day start = date(2000, 1, 1);
    std::int64_t checksum = 0;

    for (auto const& calculator : calculators) {
        const auto began = steady_clock::now();
        for (int offset = 0; offset < days; ++offset) {
            const auto t = calculator.times(year_month_day{ sys_days{ start } + std::chrono::days{ offset } }, 43.65, -79.38);
            checksum += minutesOf(t.isha);
        }
        const auto elapsed = duration<double, std::nano>(steady_clock::now() - began);
        std::println("{:<8} {:>7.1f} ns per day ({} days)", key(calculator.method), elapsed.count() / days, days);
    }
    std::println("checksum {}", checksum);
    return 0;
}

}

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view{ argv[1] } == "--bench") {
        return benchmark();
    }

    matchesPraytimeJs();
    matchesIosApp();
    ramadanFollowsUmmAlQura();
    ummAlQuraIshaIsLaterDuringRamadan();
    keysRoundTrip();
    timetableStatus();
    timetableUpcoming();
    timetableEvents();
    qiblaBearings();
    timeFormatting();

    if (failures != 0) {
        std::println("{} failures", failures);
        return 1;
    }
    std::println("All tests passed");
    return 0;
}
