#pragma once

#include "sujud/PrayTimes.h"
#include "sujud/Timetable.h"

#include <initializer_list>
#include <string>
#include <string_view>

namespace sujud::strings {

std::wstring get(std::wstring_view key);

// Replaces {0}, {1}, ... in the resource string.
std::wstring format(std::wstring_view key, std::initializer_list<std::wstring_view> arguments);

std::wstring prayer(Prayer prayer);
std::wstring method(Method method);
std::wstring asr(AsrJuristic asr);

// The language resources resolve to, such as "ar" or "en-US".
std::wstring const& language();
bool isRightToLeft();

}
