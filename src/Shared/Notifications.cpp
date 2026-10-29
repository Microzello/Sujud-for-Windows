#include "pch.h"
#include "Notifications.h"

#include "Settings.h"
#include "Strings.h"
#include "sujud/Clock.h"

#include <format>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.UI.Notifications.h>

namespace sujud::notifications {
namespace {

using namespace std::chrono;
using winrt::Windows::Data::Xml::Dom::XmlDocument;
using winrt::Windows::UI::Notifications::ScheduledToastNotification;
using winrt::Windows::UI::Notifications::ToastNotificationManager;

constexpr int days = 30;
constexpr auto refillWhenWithin = std::chrono::days{ 20 };
constexpr wchar_t queueKey[] = L"notificationQueue";
constexpr wchar_t queuedThroughKey[] = L"notificationsQueuedThrough";

std::wstring escape(std::wstring_view text) {
    std::wstring result;
    for (const auto character : text) {
        switch (character) {
        case L'&': result += L"&amp;"; break;
        case L'<': result += L"&lt;"; break;
        case L'>': result += L"&gt;"; break;
        case L'"': result += L"&quot;"; break;
        default: result.push_back(character);
        }
    }
    return result;
}

XmlDocument toast(Event const& event) {
    const auto title = strings::format(L"PrayerTimeNotification", { strings::prayer(event.prayer) });
    const auto time = clock::shortTime(clock::local(event.time));
    XmlDocument document;
    document.LoadXml(std::format(
        L"<toast launch=\"prayer={}\"><visual><binding template=\"ToastGeneric\"><text>{}</text><text>{}</text></binding></visual>"
        L"<audio src=\"ms-winsoundevent:Notification.Reminder\"/></toast>",
        std::wstring{ winrt::to_hstring(key(event.prayer)) }, escape(title), escape(time)));
    return document;
}

}

void reschedule(bool force) {
    const auto where = settings::location();
    if (!where) {
        return;
    }
    const auto calculator = settings::calculator();
    const auto now = system_clock::now();
    const auto queue = std::format(L"{}|{}|{}|{}", winrt::to_hstring(key(calculator.method)), winrt::to_hstring(key(calculator.asr)), where->latitude, where->longitude);

    const auto stored = winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Values();
    const auto storedQueue = winrt::unbox_value_or<winrt::hstring>(stored.TryLookup(queueKey), L"");
    const auto queuedThrough = system_clock::time_point{ system_clock::duration{ winrt::unbox_value_or<std::int64_t>(stored.TryLookup(queuedThroughKey), 0) } };
    if (!force && storedQueue == queue && queuedThrough - now > refillWhenWithin) {
        return;
    }

    const auto applicationId = winrt::Windows::ApplicationModel::Package::Current().Id().FamilyName() + L"!App";
    const auto notifier = ToastNotificationManager::CreateToastNotifier(applicationId);
    for (auto const& scheduled : notifier.GetScheduledToastNotifications()) {
        notifier.RemoveFromSchedule(scheduled);
    }

    const Timetable timetable{ *where, calculator };
    const auto instant = floor<seconds>(now);
    Instant lastQueued = instant;
    for (auto const& event : timetable.events(instant, days)) {
        if (!isPrayer(event.prayer) || event.time <= instant) {
            continue;
        }
        ScheduledToastNotification notification{ toast(event), winrt::clock::from_sys(event.time) };
        notification.Tag(winrt::to_hstring(std::format("{}{}", key(event.prayer), event.time.time_since_epoch().count())));
        notification.Group(L"prayers");
        notifier.AddToSchedule(notification);
        lastQueued = event.time;
    }

    stored.Insert(queueKey, winrt::box_value(winrt::hstring{ queue }));
    stored.Insert(queuedThroughKey, winrt::box_value(system_clock::time_point{ lastQueued }.time_since_epoch().count()));
}

}
