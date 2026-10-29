#pragma once

#include <chrono>
#include <string>
#include <string_view>

namespace sujud::widgets {

std::wstring const& cardTemplate(std::wstring_view definition);
std::wstring data(std::chrono::system_clock::time_point now);

void push(std::wstring_view widgetId, std::wstring_view definition);
void pushAll();

// The next whole minute, when every countdown changes.
std::chrono::system_clock::time_point nextRefresh(std::chrono::system_clock::time_point now);

}
