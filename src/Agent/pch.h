#pragma once

#include <stdint.h>
#include <combaseapi.h>

void SignalLocalServerShutdown();

// COM local servers end when the last object they serve is released, so every
// C++/WinRT object holds the server process rather than a DLL-style module count.
namespace winrt {
inline auto get_module_lock() noexcept {
    struct service_lock {
        uint32_t operator++() noexcept { return ::CoAddRefServerProcess(); }
        uint32_t operator--() noexcept {
            const auto ref = ::CoReleaseServerProcess();
            if (ref == 0) {
                SignalLocalServerShutdown();
            }
            return ref;
        }
    };
    return service_lock{};
}
}

#define WINRT_CUSTOM_MODULE_LOCK

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <wil/cppwinrt.h>
#include <wil/resource.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
