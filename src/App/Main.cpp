#include "pch.h"
#include "App.xaml.h"

#include <thread>

using namespace winrt;
using namespace winrt::Microsoft::Windows::AppLifecycle;

namespace {

// The redirect must not block this STA, so it runs on another thread while COM keeps pumping here.
void redirect(AppInstance const& primary) {
    const wil::unique_event done{ wil::EventOptions::None };
    const auto arguments = AppInstance::GetCurrent().GetActivatedEventArgs();
    std::thread worker{ [&] {
        primary.RedirectActivationToAsync(arguments).get();
        done.SetEvent();
    } };
    DWORD index = 0;
    HANDLE handles[] = { done.get() };
    CoWaitForMultipleObjects(CWMO_DEFAULT, INFINITE, 1, handles, &index);
    worker.join();
}

}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    init_apartment(apartment_type::single_threaded);

    const auto primary = AppInstance::FindOrRegisterForKey(L"main");
    if (!primary.IsCurrent()) {
        redirect(primary);
        return 0;
    }

    winrt::Microsoft::UI::Xaml::Application::Start([](auto&&) { make<Sujud::implementation::App>(); });
    return 0;
}
