#pragma once

#include "MainWindow.g.h"

namespace winrt::Sujud::implementation {

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();

    void BringToFront();
    void OnSelectionChanged(Microsoft::UI::Xaml::Controls::NavigationView const&, Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args);
    void OnItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView const&, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const& args);

private:
    fire_and_forget chooseLocation();
    bool choosing = false;
};

}

namespace winrt::Sujud::factory_implementation {

struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};

}
