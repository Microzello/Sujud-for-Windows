#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"

#include "Lifecycle.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;

namespace winrt::Sujud::implementation {

App::App() {
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const&) {
        if (IsDebuggerPresent()) {
            __debugbreak();
        }
    });
#endif
}

void App::OnLaunched(LaunchActivatedEventArgs const&) {
    Resources().MergedDictionaries().InsertAt(0, Controls::XamlControlsResources{});
    window = make<MainWindow>();
    window.Activate();

    Microsoft::Windows::AppLifecycle::AppInstance::GetCurrent().Activated([weak = make_weak(window)](auto&&, auto&&) {
        if (const auto strong = weak.get()) {
            strong.DispatcherQueue().TryEnqueue([strong] { strong.as<Sujud::MainWindow>().BringToFront(); });
        }
    });

    ::sujud::lifecycle::start();
}

}
