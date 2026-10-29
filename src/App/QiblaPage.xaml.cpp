#include "pch.h"
#include "QiblaPage.xaml.h"
#if __has_include("QiblaPage.g.cpp")
#include "QiblaPage.g.cpp"
#endif

#include "Lifecycle.h"
#include "Settings.h"
#include "Strings.h"
#include "sujud/Qibla.h"

#include <cmath>
#include <format>

#include <windows.graphics.display.interop.h>

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Microsoft::UI::Xaml::Shapes;

namespace winrt::Sujud::implementation {
namespace {

constexpr double size = 360;
constexpr double radius = size / 2;
constexpr double facingTolerance = 3;

// Compass readings follow the device's native axes, not the screen as currently rotated.
// https://learn.microsoft.com/windows/apps/develop/devices-sensors/sensor-orientation
double displayCompensation(Windows::Graphics::Display::DisplayInformation const& display) {
    using Windows::Graphics::Display::DisplayOrientations;
    const auto native = display.NativeOrientation();
    const bool portraitFirst = native == DisplayOrientations::Portrait || native == DisplayOrientations::PortraitFlipped;
    switch (display.CurrentOrientation()) {
    case DisplayOrientations::Portrait: return portraitFirst ? 0 : 270;
    case DisplayOrientations::LandscapeFlipped: return portraitFirst ? 270 : 180;
    case DisplayOrientations::PortraitFlipped: return portraitFirst ? 180 : 90;
    default: return portraitFirst ? 90 : 0;
    }
}

// A full-size layer turned about the dial's centre, holding one element at its top.
Grid spoke(UIElement const& element, double angle, double inset) {
    Grid layer;
    layer.Width(size);
    layer.Height(size);
    RotateTransform rotation;
    rotation.Angle(angle);
    rotation.CenterX(radius);
    rotation.CenterY(radius);
    layer.RenderTransform(rotation);
    if (const auto framework = element.try_as<FrameworkElement>()) {
        framework.HorizontalAlignment(HorizontalAlignment::Center);
        framework.VerticalAlignment(VerticalAlignment::Top);
        framework.Margin({ 0, inset, 0, 0 });
    }
    layer.Children().Append(element);
    return layer;
}

TextBlock upright(std::wstring const& text, Style const& style) {
    TextBlock block;
    block.Text(text);
    block.Style(style);
    block.RenderTransformOrigin({ 0.5, 0.5 });
    block.RenderTransform(RotateTransform{});
    return block;
}

}

QiblaPage::QiblaPage() {
    InitializeComponent();
    buildDial();
    compass = Windows::Devices::Sensors::Compass::GetDefault();

    const auto label = box_value(hstring{ ::sujud::strings::get(L"ReverseCompass") });
    Reverse().OnContent(label);
    Reverse().OffContent(label);
    Reverse().IsOn(::sujud::settings::compassReversed());
    Reverse().Visibility(compass ? Visibility::Visible : Visibility::Collapsed);
    Loaded([this](auto&&, auto&&) { trackDisplay(); });
}

void QiblaPage::buildDial() {
    const auto style = [this](wchar_t const* key) { return Resources().Lookup(box_value(key)).as<winrt::Microsoft::UI::Xaml::Style>(); };

    for (int tick = 0; tick < 72; ++tick) {
        Shapes::Rectangle mark;
        mark.Style(style(tick == 0 ? L"NorthTick" : tick % 6 == 0 ? L"MajorTick" : L"MinorTick"));
        Dial().Children().Append(spoke(mark, tick * 5.0, 0));
    }

    constexpr wchar_t const* keys[] = { L"CardinalN", L"CardinalE", L"CardinalS", L"CardinalW" };
    for (std::size_t index = 0; index < cardinals.size(); ++index) {
        cardinals[index] = upright(::sujud::strings::get(keys[index]), style(index == 0 ? L"North" : L"Cardinal"));
        Dial().Children().Append(spoke(cardinals[index], index * 90.0, radius * 0.22 - size * 0.035));
    }

    kaaba = upright(L"\U0001F54B", style(L"Kaaba"));
    const auto kaabaSpoke = spoke(kaaba, 0, radius * 0.44 - size * 0.045);
    kaabaBearing = kaabaSpoke.RenderTransform().as<RotateTransform>();
    Dial().Children().Append(kaabaSpoke);
}

void QiblaPage::OnNavigatedTo(Navigation::NavigationEventArgs const&) {
    subscription = ::sujud::lifecycle::subscribe([this] { reload(); });
    reload();
    if (compass) {
        compass.ReportInterval(std::max<uint32_t>(compass.MinimumReportInterval(), 16));
        readingToken = compass.ReadingChanged([weak = get_weak()](auto&&, Windows::Devices::Sensors::CompassReadingChangedEventArgs const& args) {
            const auto reading = args.Reading();
            const auto value = reading.HeadingTrueNorth() ? reading.HeadingTrueNorth().Value() : reading.HeadingMagneticNorth();
            if (const auto self = weak.get()) {
                self->DispatcherQueue().TryEnqueue([weak, value] {
                    if (const auto page = weak.get()) {
                        page->apply(value);
                    }
                });
            }
        });
    }
}

void QiblaPage::OnNavigatedFrom(Navigation::NavigationEventArgs const&) {
    ::sujud::lifecycle::unsubscribe(subscription);
    if (compass) {
        compass.ReadingChanged(readingToken);
        compass.ReportInterval(0);
    }
    if (display) {
        display.OrientationChanged(orientationToken);
        display = nullptr;
    }
}

void QiblaPage::OnReverseToggled(IInspectable const&, RoutedEventArgs const&) {
    ::sujud::settings::setCompassReversed(Reverse().IsOn());
    if (lastReading) {
        apply(*lastReading);
    }
}

void QiblaPage::trackDisplay() {
    if (!compass || display || !XamlRoot()) {
        return;
    }
    try {
        const auto window = Microsoft::UI::GetWindowFromWindowId(XamlRoot().ContentIslandEnvironment().AppWindowId());
        const auto factory = get_activation_factory<Windows::Graphics::Display::DisplayInformation, IDisplayInformationStaticsInterop>();
        check_hresult(factory->GetForWindow(window, guid_of<Windows::Graphics::Display::DisplayInformation>(), put_abi(display)));
        headingOffset = displayCompensation(display);
        orientationToken = display.OrientationChanged([this](auto&&, auto&&) {
            headingOffset = displayCompensation(display);
            if (lastReading) {
                apply(*lastReading);
            }
        });
    } catch (...) {
        display = nullptr;
        headingOffset = 0;
    }
}

void QiblaPage::reload() {
    const auto where = ::sujud::settings::location();
    qibla = where ? ::sujud::qibla::bearing(where->latitude, where->longitude) : 0;
    Bearing().Text(where ? std::format(L"{}°", std::lround(qibla)) : L"-");
    kaaba.Visibility(where ? Visibility::Visible : Visibility::Collapsed);
    kaabaBearing.Angle(qibla);
    render();
}

void QiblaPage::apply(double reading) {
    lastReading = reading;
    const double facing = std::fmod(reading + headingOffset + (Reverse().IsOn() ? 180 : 0), 360);
    if (!heading) {
        heading = facing;
    } else {
        heading = *heading + ::sujud::qibla::angleDifference(std::fmod(*heading, 360), facing);
    }
    render();
}

void QiblaPage::render() {
    const double facingAngle = heading.value_or(0);
    DialRotation().Angle(-facingAngle);
    ArrowRotation().Angle(qibla - facingAngle);
    for (std::size_t index = 0; index < cardinals.size(); ++index) {
        cardinals[index].RenderTransform().as<RotateTransform>().Angle(facingAngle - index * 90.0);
    }
    kaaba.RenderTransform().as<RotateTransform>().Angle(facingAngle - qibla);

    const bool facing = heading && std::abs(::sujud::qibla::angleDifference(std::fmod(*heading, 360), qibla)) < facingTolerance;
    VisualStateManager::GoToState(*this, facing ? L"Facing" : L"NotFacing", false);
}

}
