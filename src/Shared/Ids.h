#pragma once

#include <guiddef.h>
#include <string_view>

namespace sujud::ids {

// Must match the com:Class and CreateInstance ClassId entries in Package.appxmanifest.
// {08A11CE9-C995-4D86-996E-80BAEC40F021}
inline constexpr GUID widgetProvider{ 0x08a11ce9, 0xc995, 0x4d86, { 0x99, 0x6e, 0x80, 0xba, 0xec, 0x40, 0xf0, 0x21 } };
// {DE033780-1FE8-4071-B0CA-10C64A827C04}
inline constexpr GUID refreshTask{ 0xde033780, 0x1fe8, 0x4071, { 0xb0, 0xca, 0x10, 0xc6, 0x4a, 0x82, 0x7c, 0x04 } };

inline constexpr std::wstring_view refreshTaskName = L"SujudRefresh";
inline constexpr std::wstring_view nextPrayerWidget = L"NextPrayer";
inline constexpr std::wstring_view prayerTimesWidget = L"PrayerTimes";

}
