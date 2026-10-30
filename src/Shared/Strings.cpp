#include "pch.h"
#include "Strings.h"

#include <winrt/Windows.Globalization.h>

#ifdef SUJUD_UWP_RESOURCES
#include <winrt/Windows.ApplicationModel.Resources.h>
#else
#include <winrt/Microsoft.Windows.ApplicationModel.Resources.h>
#endif

namespace sujud::strings {
namespace {

#ifdef SUJUD_UWP_RESOURCES
winrt::Windows::ApplicationModel::Resources::ResourceLoader const& loader() {
    static const auto instance = winrt::Windows::ApplicationModel::Resources::ResourceLoader::GetForViewIndependentUse();
    return instance;
}
#else
winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceLoader const& loader() {
    static const winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceLoader instance;
    return instance;
}
#endif

}

std::wstring get(std::wstring_view key) {
    try {
        return std::wstring{ loader().GetString(winrt::hstring{ key }) };
    } catch (...) {
        return std::wstring{ key };
    }
}

std::wstring format(std::wstring_view key, std::initializer_list<std::wstring_view> arguments) {
    auto text = get(key);
    std::size_t index = 0;
    for (const auto argument : arguments) {
        const auto placeholder = L"{" + std::to_wstring(index++) + L"}";
        if (const auto at = text.find(placeholder); at != std::wstring::npos) {
            text.replace(at, placeholder.size(), argument);
        }
    }
    return text;
}

std::wstring prayer(Prayer prayer) {
    switch (prayer) {
    case Prayer::Fajr: return get(L"PrayerFajr");
    case Prayer::Sunrise: return get(L"PrayerSunrise");
    case Prayer::Dhuhr: return get(L"PrayerDhuhr");
    case Prayer::Asr: return get(L"PrayerAsr");
    case Prayer::Maghrib: return get(L"PrayerMaghrib");
    case Prayer::Isha: return get(L"PrayerIsha");
    }
    return {};
}

std::wstring method(Method method) {
    return get(L"Method" + winrt::to_hstring(key(method)));
}

std::wstring asr(AsrJuristic asr) {
    return get(L"Asr" + winrt::to_hstring(key(asr)));
}

std::wstring const& language() {
    static const std::wstring resolved = [] {
        try {
            const auto languages = winrt::Windows::Globalization::ApplicationLanguages::Languages();
            if (languages.Size() > 0) {
                return std::wstring{ languages.GetAt(0) };
            }
        } catch (...) {
        }
        return std::wstring{ L"en-US" };
    }();
    return resolved;
}

bool isRightToLeft() {
    try {
        return winrt::Windows::Globalization::Language{ language() }.LayoutDirection() == winrt::Windows::Globalization::LanguageLayoutDirection::Rtl;
    } catch (...) {
        return false;
    }
}

}
