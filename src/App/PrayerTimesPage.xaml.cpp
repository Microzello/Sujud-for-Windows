#include "pch.h"
#include "PrayerTimesPage.xaml.h"
#if __has_include("PrayerTimesPage.g.cpp")
#include "PrayerTimesPage.g.cpp"
#endif

#include "Lifecycle.h"
#include "Settings.h"
#include "Strings.h"
#include "sujud/Clock.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace std::chrono;

namespace winrt::Sujud::implementation {

PrayerTimesPage::PrayerTimesPage() {
    InitializeComponent();

    const auto style = [this](wchar_t const* key) { return Resources().Lookup(box_value(key)).as<winrt::Microsoft::UI::Xaml::Style>(); };
    const auto label = [&](wchar_t const* key) {
        TextBlock block;
        block.Style(style(key));
        return block;
    };
    const auto separator = [&] {
        Border line;
        line.Style(style(L"Separator"));
        return line;
    };

    Rows().Children().Append(separator());
    for (const auto prayer : ::sujud::prayers) {
        auto& row = rows[static_cast<std::size_t>(prayer)];
        row.caption = label(L"CaptionText");
        row.name = label(L"NameText");
        row.name.Text(::sujud::strings::prayer(prayer));
        row.time = label(L"TimeText");
        row.dayPeriod = label(L"DayPeriodText");

        StackPanel left;
        left.Spacing(6);
        left.VerticalAlignment(VerticalAlignment::Bottom);
        left.Children().Append(row.caption);
        left.Children().Append(row.name);

        StackPanel right;
        right.Orientation(Orientation::Horizontal);
        right.Spacing(4);
        right.VerticalAlignment(VerticalAlignment::Bottom);
        right.Children().Append(row.time);
        right.Children().Append(row.dayPeriod);

        Grid grid;
        grid.Padding({ 0, 10, 0, 14 });
        grid.ColumnDefinitions().Append(ColumnDefinition{});
        ColumnDefinition timeColumn;
        timeColumn.Width(GridLengthHelper::Auto());
        grid.ColumnDefinitions().Append(timeColumn);
        grid.Children().Append(left);
        grid.Children().Append(right);
        Grid::SetColumn(right, 1);

        Rows().Children().Append(grid);
        Rows().Children().Append(separator());
    }

    timer = DispatcherQueue().CreateTimer();
    timer.IsRepeating(false);
    timer.Tick([this](auto&&, auto&&) {
        tick();
        scheduleTick();
    });
}

void PrayerTimesPage::OnNavigatedTo(Navigation::NavigationEventArgs const&) {
    subscription = ::sujud::lifecycle::subscribe([this] { reload(); });
    reload();
}

void PrayerTimesPage::OnNavigatedFrom(Navigation::NavigationEventArgs const&) {
    ::sujud::lifecycle::unsubscribe(subscription);
    timer.Stop();
}

void PrayerTimesPage::reload() {
    timetable = ::sujud::settings::timetable();
    tick();
    scheduleTick();
}

void PrayerTimesPage::scheduleTick() {
    const auto now = system_clock::now();
    timer.Interval(duration_cast<Windows::Foundation::TimeSpan>(ceil<seconds>(now + 1ms) - now));
    timer.Start();
}

void PrayerTimesPage::tick() {
    const auto now = system_clock::now();
    const auto instant = floor<seconds>(now);

    if (!timetable) {
        Heading().Text(::sujud::strings::get(L"PrayerTimes"));
        for (auto& row : rows) {
            row.caption.Text(L" ");
            row.time.Text(L"-");
            row.dayPeriod.Text(L"");
        }
        return;
    }

    const auto status = timetable->status(instant);
    if (status.next) {
        Heading().Text(::sujud::strings::format(L"NextPrayerIn", { ::sujud::strings::prayer(status.next->prayer), ::sujud::clock::countdown(now, status.next->time) }));
    } else {
        Heading().Text(::sujud::strings::get(L"PrayerTimes"));
    }

    const auto today = timetable->dateOf(instant);
    const auto upcoming = timetable->upcoming(instant);
    for (const auto prayer : ::sujud::prayers) {
        auto& row = rows[static_cast<std::size_t>(prayer)];
        const auto& event = upcoming[static_cast<std::size_t>(prayer)];
        if (!event) {
            row.caption.Text(L" ");
            row.time.Text(L"-");
            row.dayPeriod.Text(L"");
            continue;
        }
        const auto countdown = ::sujud::clock::countdown(now, event->time);
        row.caption.Text(timetable->dateOf(event->time) == today ? countdown : ::sujud::strings::format(L"Tomorrow", { countdown }));
        const auto wall = ::sujud::clock::local(event->time);
        row.time.Text(::sujud::clock::hourMinute(wall));
        row.dayPeriod.Text(::sujud::clock::dayPeriod(wall).value_or(L""));
    }
}

}
