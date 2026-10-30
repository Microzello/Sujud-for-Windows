#include "pch.h"
#include "Widget.h"

#include <mutex>
#include <vector>

namespace {

using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::UI::Xaml;
using sujud::gamebar::Widget;

struct App : ApplicationT<App> {
    App() {
        Suspending([this](auto&&, auto&&) {
            std::scoped_lock lock{ mutex };
            for (auto const& widget : widgets) {
                widget->stop();
            }
            widgets.clear();
        });
    }

    void OnLaunched(LaunchActivatedEventArgs const&) {
        Exit();
    }

    void OnActivated(IActivatedEventArgs const& args) {
        const auto widgetArgs = args.try_as<winrt::Microsoft::Gaming::XboxGameBar::XboxGameBarWidgetActivatedEventArgs>();
        if (!widgetArgs || !widgetArgs.IsLaunchActivation()) {
            return;
        }

        auto widget = winrt::make_self<Widget>();
        widget->start(widgetArgs);
        Window::Current().Closed([this, raw = widget.get()](auto&&, auto&&) {
            std::scoped_lock lock{ mutex };
            std::erase_if(widgets, [raw](auto const& held) {
                if (held.get() != raw) {
                    return false;
                }
                held->stop();
                return true;
            });
        });

        std::scoped_lock lock{ mutex };
        widgets.push_back(std::move(widget));
    }

private:
    std::mutex mutex;
    std::vector<winrt::com_ptr<Widget>> widgets;
};

}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    Application::Start([](auto&&) { winrt::make<App>(); });
    return 0;
}
