#pragma once

#include "sujud/Timetable.h"

#include <optional>
#include <vector>

#include <winrt/Windows.UI.Composition.h>

namespace sujud::gamebar {

struct Widget : winrt::implements<Widget, winrt::Windows::Foundation::IInspectable> {
    void start(winrt::Microsoft::Gaming::XboxGameBar::XboxGameBarWidgetActivatedEventArgs const& args);
    void stop();

private:
    winrt::Windows::UI::Xaml::UIElement build();
    winrt::Windows::UI::Xaml::UIElement shadowed(winrt::Windows::UI::Xaml::Controls::TextBlock const& text);
    void refresh();
    void applyTheme();
    void notifyIfDue(Status const& status, Instant now);
    winrt::fire_and_forget notify(Event event);
    winrt::fire_and_forget onThemeChanged(winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IInspectable);

    winrt::Microsoft::Gaming::XboxGameBar::XboxGameBarWidget widget{ nullptr };
    winrt::Microsoft::Gaming::XboxGameBar::XboxGameBarWidgetNotificationManager notifications{ nullptr };
    winrt::Windows::UI::Xaml::DispatcherTimer timer;
    winrt::Microsoft::Gaming::XboxGameBar::XboxGameBarWidget::RequestedThemeChanged_revoker themeChanged;

    winrt::Windows::UI::Xaml::Controls::StackPanel root;
    winrt::Windows::UI::Xaml::Controls::StackPanel ready;
    winrt::Windows::UI::Xaml::Controls::HyperlinkButton unset;
    winrt::Windows::UI::Xaml::Controls::TextBlock title;
    winrt::Windows::UI::Xaml::Controls::TextBlock at;
    std::vector<winrt::Windows::UI::Composition::DropShadow> shadows;

    Instant opened;
    std::optional<Instant> notified;
};

}
