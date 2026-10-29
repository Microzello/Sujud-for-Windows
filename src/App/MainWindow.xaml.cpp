#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "PrayerTimesPage.xaml.h"
#include "QiblaPage.xaml.h"
#include "SettingsPage.xaml.h"

#include "Lifecycle.h"
#include "Settings.h"
#include "Strings.h"

#include <cmath>

#include <winrt/Windows.Devices.Geolocation.h>

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media::Animation;

namespace winrt::Sujud::implementation {
namespace {

using namespace std::chrono;

IInspectable text(std::wstring_view key) {
    return box_value(hstring{ ::sujud::strings::get(key) });
}

Windows::Foundation::IAsyncOperation<bool> useDeviceLocation() {
    using namespace Windows::Devices::Geolocation;
    try {
        if (co_await Geolocator::RequestAccessAsync() != GeolocationAccessStatus::Allowed) {
            co_return false;
        }
        Geolocator locator;
        locator.DesiredAccuracyInMeters(1000);
        const auto position = co_await locator.GetGeopositionAsync(minutes{ 5 }, seconds{ 20 });
        const auto point = position.Coordinate().Point().Position();
        ::sujud::settings::setLocation({ point.Latitude, point.Longitude });
        co_return true;
    } catch (...) {
        co_return false;
    }
}

}

MainWindow::MainWindow() {
    InitializeComponent();

    ExtendsContentIntoTitleBar(true);
    SetTitleBar(AppTitleBar());
    Title(L"Sujud");

    const auto window = AppWindow();
    const auto scale = GetDpiForWindow(Microsoft::UI::GetWindowFromWindowId(window.Id())) / 96.0;
    window.Resize({ static_cast<int32_t>(480 * scale), static_cast<int32_t>(780 * scale) });
    if (const auto presenter = window.Presenter().try_as<Microsoft::UI::Windowing::OverlappedPresenter>()) {
        presenter.PreferredMinimumWidth(static_cast<int32_t>(360 * scale));
        presenter.PreferredMinimumHeight(static_cast<int32_t>(480 * scale));
    }

    Root().FlowDirection(::sujud::strings::isRightToLeft() ? FlowDirection::RightToLeft : FlowDirection::LeftToRight);
    TimesItem().Content(text(L"PrayerTimes"));
    QiblaItem().Content(text(L"Qibla"));
    LocationItem().Content(text(L"Location"));
    Navigation().SelectedItem(TimesItem());

    Root().Loaded([this](auto&&, auto&&) {
        if (!::sujud::settings::location()) {
            chooseLocation();
        }
    });
}

void MainWindow::BringToFront() {
    Activate();
    SetForegroundWindow(Microsoft::UI::GetWindowFromWindowId(AppWindow().Id()));
}

void MainWindow::OnSelectionChanged(NavigationView const&, NavigationViewSelectionChangedEventArgs const& args) {
    const SuppressNavigationTransitionInfo transition;
    if (args.IsSettingsSelected()) {
        ContentFrame().Navigate(xaml_typename<Sujud::SettingsPage>(), nullptr, transition);
        return;
    }
    const auto item = args.SelectedItem().try_as<NavigationViewItem>();
    if (item == QiblaItem()) {
        ContentFrame().Navigate(xaml_typename<Sujud::QiblaPage>(), nullptr, transition);
    } else {
        ContentFrame().Navigate(xaml_typename<Sujud::PrayerTimesPage>(), nullptr, transition);
    }
}

void MainWindow::OnItemInvoked(NavigationView const&, NavigationViewItemInvokedEventArgs const& args) {
    if (args.InvokedItemContainer() == LocationItem()) {
        chooseLocation();
    }
}

fire_and_forget MainWindow::chooseLocation() {
    if (choosing) {
        co_return;
    }
    choosing = true;
    const auto strong = get_strong();
    const auto saved = ::sujud::settings::location();

    NumberBox latitude;
    latitude.Header(text(L"Latitude"));
    latitude.Minimum(-90);
    latitude.Maximum(90);
    NumberBox longitude;
    longitude.Header(text(L"Longitude"));
    longitude.Minimum(-180);
    longitude.Maximum(180);
    if (saved) {
        latitude.Value(saved->latitude);
        longitude.Value(saved->longitude);
    }

    InfoBar problem;
    problem.Severity(InfoBarSeverity::Warning);
    problem.IsClosable(false);
    problem.Title(::sujud::strings::get(L"LocationAccessNeeded"));

    StackPanel fields;
    fields.Spacing(12);
    fields.Children().Append(latitude);
    fields.Children().Append(longitude);
    fields.Children().Append(problem);

    ContentDialog dialog;
    dialog.XamlRoot(Content().XamlRoot());
    dialog.FlowDirection(Root().FlowDirection());
    dialog.Title(text(L"Location"));
    dialog.Content(fields);
    dialog.PrimaryButtonText(::sujud::strings::get(L"SetLocation"));
    dialog.SecondaryButtonText(::sujud::strings::get(L"CurrentLocation"));
    if (saved) {
        dialog.CloseButtonText(::sujud::strings::get(L"Cancel"));
    }
    dialog.DefaultButton(ContentDialogButton::Primary);

    dialog.PrimaryButtonClick([latitude, longitude](ContentDialog const&, ContentDialogButtonClickEventArgs const& args) {
        if (std::isnan(latitude.Value()) || std::isnan(longitude.Value())) {
            args.Cancel(true);
            return;
        }
        ::sujud::settings::setLocation({ latitude.Value(), longitude.Value() });
        ::sujud::lifecycle::settingsChanged();
    });

    dialog.SecondaryButtonClick([problem](ContentDialog const&, ContentDialogButtonClickEventArgs args) -> fire_and_forget {
        const auto deferral = args.GetDeferral();
        problem.IsOpen(false);
        if (co_await useDeviceLocation()) {
            ::sujud::lifecycle::settingsChanged();
        } else {
            args.Cancel(true);
            problem.IsOpen(true);
        }
        deferral.Complete();
    });

    co_await dialog.ShowAsync();
    choosing = false;
}

}
