#pragma once

#include "QiblaPage.g.h"

#include <array>
#include <optional>

#include <winrt/Windows.Devices.Sensors.h>
#include <winrt/Windows.Graphics.Display.h>

namespace winrt::Sujud::implementation {

struct QiblaPage : QiblaPageT<QiblaPage> {
    QiblaPage();

    void OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&);
    void OnNavigatedFrom(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const&);
    void OnReverseToggled(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

private:
    void buildDial();
    void reload();
    void trackDisplay();
    void apply(double reading);
    void render();

    double qibla = 0;
    // Unwrapped, so turning past north never spins the dial the long way round.
    std::optional<double> heading;
    std::optional<double> lastReading;
    double headingOffset = 0;
    std::size_t subscription = 0;
    Windows::Devices::Sensors::Compass compass{ nullptr };
    event_token readingToken{};
    Windows::Graphics::Display::DisplayInformation display{ nullptr };
    event_token orientationToken{};
    Microsoft::UI::Xaml::Controls::TextBlock kaaba{ nullptr };
    Microsoft::UI::Xaml::Media::RotateTransform kaabaBearing{ nullptr };
    std::array<Microsoft::UI::Xaml::Controls::TextBlock, 4> cardinals{ nullptr, nullptr, nullptr, nullptr };
};

}

namespace winrt::Sujud::factory_implementation {

struct QiblaPage : QiblaPageT<QiblaPage, implementation::QiblaPage> {};

}
