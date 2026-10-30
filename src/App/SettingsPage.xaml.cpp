#include "pch.h"
#include "SettingsPage.xaml.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

#include "Lifecycle.h"
#include "Settings.h"
#include "Strings.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::Sujud::implementation {

SettingsPage::SettingsPage() {
    InitializeComponent();

    const auto current = ::sujud::settings::calculator();

    MethodBox().Header(box_value(hstring{ ::sujud::strings::get(L"CalculationMethod") }));
    for (const auto method : ::sujud::methods) {
        MethodBox().Items().Append(box_value(hstring{ ::sujud::strings::method(method) }));
    }
    MethodBox().SelectedIndex(static_cast<int32_t>(current.method));

    AsrButtons().Header(box_value(hstring{ ::sujud::strings::get(L"AsrMethod") }));
    for (const auto asr : ::sujud::asrJuristics) {
        AsrButtons().Items().Append(box_value(hstring{ ::sujud::strings::asr(asr) }));
    }
    AsrButtons().SelectedIndex(static_cast<int32_t>(current.asr));

    FullScreenTimes().Header(box_value(hstring{ ::sujud::strings::get(L"FullScreen") }));
    const auto showTimes = box_value(hstring{ ::sujud::strings::get(L"ShowPrayerTimes") });
    FullScreenTimes().OnContent(showTimes);
    FullScreenTimes().OffContent(showTimes);
    FullScreenTimes().IsOn(::sujud::settings::fullScreenTimes());

    loading = false;
}

void SettingsPage::OnMethodChanged(IInspectable const&, SelectionChangedEventArgs const&) {
    const auto index = MethodBox().SelectedIndex();
    if (loading || index < 0) {
        return;
    }
    ::sujud::settings::setMethod(::sujud::methods[static_cast<std::size_t>(index)]);
    ::sujud::lifecycle::settingsChanged();
}

void SettingsPage::OnAsrChanged(IInspectable const&, SelectionChangedEventArgs const&) {
    const auto index = AsrButtons().SelectedIndex();
    if (loading || index < 0) {
        return;
    }
    ::sujud::settings::setAsr(::sujud::asrJuristics[static_cast<std::size_t>(index)]);
    ::sujud::lifecycle::settingsChanged();
}

void SettingsPage::OnFullScreenTimesToggled(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
    if (!loading) {
        ::sujud::settings::setFullScreenTimes(FullScreenTimes().IsOn());
    }
}

}
