#include "pch.h"

#include "Ids.h"
#include "Notifications.h"
#include "WidgetContent.h"
#include "WidgetProvider.h"

#include <mutex>

#include <winrt/Windows.ApplicationModel.Background.h>

namespace {

wil::unique_event shutdownEvent{ wil::EventOptions::ManualReset };

struct RefreshTask : winrt::implements<RefreshTask, winrt::Windows::ApplicationModel::Background::IBackgroundTask> {
    void Run(winrt::Windows::ApplicationModel::Background::IBackgroundTaskInstance const&) {
        try {
            sujud::notifications::reschedule();
            sujud::widgets::pushAll();
        } catch (...) {
            LOG_CAUGHT_EXCEPTION();
        }
    }
};

template <typename T>
struct ClassFactory : winrt::implements<ClassFactory<T>, IClassFactory, winrt::no_module_lock> {
    STDMETHODIMP CreateInstance(::IUnknown* outer, GUID const& iid, void** result) noexcept final {
        if (!result) {
            return E_POINTER;
        }
        *result = nullptr;
        if (outer) {
            return CLASS_E_NOAGGREGATION;
        }
        try {
            std::scoped_lock lock{ mutex };
            return winrt::make<T>().as(iid, result);
        } catch (...) {
            return winrt::to_hresult();
        }
    }

    STDMETHODIMP LockServer(BOOL) noexcept final {
        return S_OK;
    }

private:
    std::mutex mutex;
};

template <typename T>
wil::unique_com_class_object_cookie registerClass(GUID const& clsid) {
    wil::unique_com_class_object_cookie cookie;
    winrt::check_hresult(CoRegisterClassObject(clsid, winrt::make<ClassFactory<T>>().get(), CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, cookie.put()));
    return cookie;
}

}

void SignalLocalServerShutdown() {
    shutdownEvent.SetEvent();
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    winrt::init_apartment();

    const auto widgetProvider = registerClass<sujud::WidgetProvider>(sujud::ids::widgetProvider);
    const auto refreshTask = registerClass<RefreshTask>(sujud::ids::refreshTask);

    DWORD index = 0;
    HANDLE events[] = { shutdownEvent.get() };
    winrt::check_hresult(CoWaitForMultipleObjects(CWMO_DISPATCH_CALLS | CWMO_DISPATCH_WINDOW_MESSAGES, INFINITE,
                                                  static_cast<ULONG>(std::size(events)), events, &index));
    return 0;
}
