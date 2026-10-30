#pragma once

#include "SettingsPage.g.h"

namespace winrt::Sujud::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
    SettingsPage();

    void OnMethodChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
    void OnAsrChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
    void OnFullScreenTimesToggled(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

private:
    bool loading = true;
};

}

namespace winrt::Sujud::factory_implementation {

struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {};

}
