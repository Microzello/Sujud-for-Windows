#pragma once

#include <winrt/Microsoft.Windows.Widgets.Providers.h>

namespace sujud {

struct WidgetProvider : winrt::implements<WidgetProvider, winrt::Microsoft::Windows::Widgets::Providers::IWidgetProvider> {
    WidgetProvider();
    ~WidgetProvider();

    void CreateWidget(winrt::Microsoft::Windows::Widgets::Providers::WidgetContext const& context);
    void DeleteWidget(winrt::hstring const& widgetId, winrt::hstring const& customState);
    void OnActionInvoked(winrt::Microsoft::Windows::Widgets::Providers::WidgetActionInvokedArgs const& args);
    void OnWidgetContextChanged(winrt::Microsoft::Windows::Widgets::Providers::WidgetContextChangedArgs const& args);
    void Activate(winrt::Microsoft::Windows::Widgets::Providers::WidgetContext const& context);
    void Deactivate(winrt::hstring const& widgetId);
};

}
