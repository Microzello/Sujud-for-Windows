#pragma once

#include "PrayerTimesPage.g.h"

#include "sujud/Timetable.h"

#include <array>
#include <optional>

namespace winrt::Sujud::implementation {

struct PrayerTimesPage : PrayerTimesPageT<PrayerTimesPage> {
    PrayerTimesPage();

    void OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&);
    void OnNavigatedFrom(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&);

private:
    struct Row {
        Microsoft::UI::Xaml::Controls::TextBlock caption{ nullptr };
        Microsoft::UI::Xaml::Controls::TextBlock name{ nullptr };
        Microsoft::UI::Xaml::Controls::TextBlock time{ nullptr };
        Microsoft::UI::Xaml::Controls::TextBlock dayPeriod{ nullptr };
    };

    void reload();
    void tick();
    void scheduleTick();

    std::array<Row, ::sujud::prayers.size()> rows;
    std::optional<::sujud::Timetable> timetable;
    Microsoft::UI::Dispatching::DispatcherQueueTimer timer{ nullptr };
    std::size_t subscription = 0;
};

}

namespace winrt::Sujud::factory_implementation {

struct PrayerTimesPage : PrayerTimesPageT<PrayerTimesPage, implementation::PrayerTimesPage> {};

}
