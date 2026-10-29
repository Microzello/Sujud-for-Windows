#include "pch.h"
#include "Lifecycle.h"

#include "Ids.h"
#include "Notifications.h"
#include "Settings.h"
#include "WidgetContent.h"

#include <cmath>
#include <map>

#include <winrt/Windows.ApplicationModel.Background.h>
#include <winrt/Windows.Devices.Geolocation.h>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>

namespace sujud::lifecycle {
namespace {

using namespace std::chrono;
using namespace winrt::Windows::ApplicationModel::Background;
using winrt::Microsoft::UI::Dispatching::DispatcherQueue;

std::map<std::size_t, std::function<void()>>& handlers() {
    static std::map<std::size_t, std::function<void()>> instance;
    return instance;
}

void notify() {
    const auto snapshot = handlers();
    for (auto const& [token, handler] : snapshot) {
        handler();
    }
}

winrt::fire_and_forget refreshQueues(bool force) {
    co_await winrt::resume_background();
    try {
        notifications::reschedule(force);
    } catch (...) {
        LOG_CAUGHT_EXCEPTION();
    }
    try {
        widgets::pushAll();
    } catch (...) {
        LOG_CAUGHT_EXCEPTION();
    }
}

winrt::fire_and_forget registerRefreshTask() {
    co_await winrt::resume_background();
    try {
        for (auto const& task : BackgroundTaskRegistration::AllTasks()) {
            if (task.Value().Name() == ids::refreshTaskName) {
                co_return;
            }
        }
        const auto access = co_await BackgroundExecutionManager::RequestAccessAsync();
        if (access == BackgroundAccessStatus::DeniedByUser || access == BackgroundAccessStatus::DeniedBySystemPolicy) {
            co_return;
        }
        BackgroundTaskBuilder builder;
        builder.Name(ids::refreshTaskName);
        builder.SetTrigger(TimeTrigger{ 15, false });
        builder.SetTaskEntryPointClsid(reinterpret_cast<winrt::guid const&>(ids::refreshTask));
        builder.Register();
    } catch (...) {
        LOG_CAUGHT_EXCEPTION();
    }
}

// Follows the device while travelling, without ever asking for permission here.
winrt::fire_and_forget followDevice(DispatcherQueue ui) {
    using namespace winrt::Windows::Devices::Geolocation;
    using namespace winrt::Windows::Security::Authorization::AppCapabilityAccess;

    co_await winrt::resume_background();
    try {
        if (AppCapability::Create(L"location").CheckAccess() != AppCapabilityAccessStatus::Allowed) {
            co_return;
        }
        Geolocator locator;
        locator.DesiredAccuracyInMeters(3000);
        const auto position = co_await locator.GetGeopositionAsync(hours{ 1 }, seconds{ 15 });
        const auto point = position.Coordinate().Point().Position();
        const Coordinates found{ point.Latitude, point.Longitude };
        const auto saved = settings::location();
        if (saved && std::abs(saved->latitude - found.latitude) < 0.01 && std::abs(saved->longitude - found.longitude) < 0.01) {
            co_return;
        }
        settings::setLocation(found);
        ui.TryEnqueue([] { settingsChanged(); });
    } catch (...) {
        LOG_CAUGHT_EXCEPTION();
    }
}

}

void start() {
    registerRefreshTask();
    refreshQueues(false);
    followDevice(DispatcherQueue::GetForCurrentThread());
}

void settingsChanged() {
    notify();
    refreshQueues(true);
}

std::size_t subscribe(std::function<void()> handler) {
    static std::size_t next = 0;
    handlers().emplace(++next, std::move(handler));
    return next;
}

void unsubscribe(std::size_t token) {
    handlers().erase(token);
}

}
