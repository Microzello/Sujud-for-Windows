#include "pch.h"
#include "WidgetContent.h"

#include "Ids.h"
#include "Settings.h"
#include "Strings.h"
#include "sujud/Clock.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>

#include <winrt/Microsoft.Windows.Widgets.Providers.h>
#include <winrt/Windows.Data.Json.h>

namespace sujud::widgets {
namespace {

using namespace std::chrono;
using winrt::Windows::Data::Json::JsonArray;
using winrt::Windows::Data::Json::JsonObject;
using winrt::Windows::Data::Json::JsonValue;

std::filesystem::path moduleDirectory() {
    std::wstring path(MAX_PATH, L'\0');
    path.resize(GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size())));
    return std::filesystem::path{ path }.parent_path();
}

std::wstring readUtf8(std::filesystem::path const& path) {
    std::ifstream file{ path, std::ios::binary };
    std::stringstream contents;
    contents << file.rdbuf();
    return std::wstring{ winrt::to_hstring(contents.str()) };
}

JsonValue string(std::wstring_view text) {
    return JsonValue::CreateStringValue(winrt::hstring{ text });
}

std::wstring timeOf(Instant instant) {
    return clock::shortTime(clock::local(instant));
}

}

std::wstring const& cardTemplate(std::wstring_view definition) {
    static std::mutex mutex;
    static std::map<std::wstring, std::wstring, std::less<>> cache;
    std::scoped_lock lock{ mutex };
    if (const auto found = cache.find(definition); found != cache.end()) {
        return found->second;
    }
    const auto path = moduleDirectory() / L"Widgets" / (std::wstring{ definition } + L".json");
    return cache.emplace(std::wstring{ definition }, readUtf8(path)).first->second;
}

std::wstring data(system_clock::time_point now) {
    JsonObject root;
    root.SetNamedValue(L"rtl", JsonValue::CreateBooleanValue(strings::isRightToLeft()));
    root.SetNamedValue(L"openApp", string(strings::get(L"OpenSujud")));

    const auto timetable = settings::timetable();
    const auto instant = floor<seconds>(now);
    const auto status = timetable ? timetable->status(instant) : Status{};
    if (!timetable || !status.next) {
        root.SetNamedValue(L"ready", JsonValue::CreateBooleanValue(false));
        return std::wstring{ root.Stringify() };
    }

    const auto& next = *status.next;
    const auto remaining = ceil<minutes>(next.time - now);
    const auto name = strings::prayer(next.prayer);
    const auto in = clock::narrowDuration(remaining, strings::language());
    const auto at = timeOf(next.time);

    root.SetNamedValue(L"ready", JsonValue::CreateBooleanValue(true));
    root.SetNamedValue(L"title", string(strings::format(L"NextPrayerIn", { name, in })));
    root.SetNamedValue(L"name", string(name));
    root.SetNamedValue(L"in", string(strings::format(L"InDuration", { in })));
    root.SetNamedValue(L"at", string(strings::format(L"AtTime", { at })));

    JsonArray rows;
    for (auto const& event : timetable->upcoming(instant)) {
        if (!event) continue;
        const bool isNext = event->prayer == next.prayer;
        JsonObject row;
        row.SetNamedValue(L"name", string(strings::prayer(event->prayer)));
        row.SetNamedValue(L"time", string(timeOf(event->time)));
        row.SetNamedValue(L"weight", string(isNext ? L"Bolder" : L"Default"));
        rows.Append(row);
    }
    root.SetNamedValue(L"times", rows);
    return std::wstring{ root.Stringify() };
}

void push(std::wstring_view widgetId, std::wstring_view definition) {
    using namespace winrt::Microsoft::Windows::Widgets::Providers;
    WidgetUpdateRequestOptions options{ winrt::hstring{ widgetId } };
    options.Template(winrt::hstring{ cardTemplate(definition) });
    options.Data(winrt::hstring{ data(system_clock::now()) });
    options.IsPlaceholderContent(!settings::location());
    WidgetManager::GetDefault().UpdateWidget(options);
}

void pushAll() {
    using namespace winrt::Microsoft::Windows::Widgets::Providers;
    for (auto const& info : WidgetManager::GetDefault().GetWidgetInfos()) {
        const auto context = info.WidgetContext();
        push(context.Id(), context.DefinitionId());
    }
}

system_clock::time_point nextRefresh(system_clock::time_point now) {
    return floor<minutes>(now) + 1min;
}

}
