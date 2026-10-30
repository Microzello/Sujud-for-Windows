#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string_view>

#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.Collections.h>

namespace sujud::mcp {

struct InvalidArguments : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Empty when no tool has that name. Throws InvalidArguments for arguments the tool can't use.
std::optional<winrt::Windows::Data::Json::JsonObject> call(
    std::wstring_view tool,
    winrt::Windows::Data::Json::JsonObject const& arguments,
    std::chrono::system_clock::time_point now);

}
