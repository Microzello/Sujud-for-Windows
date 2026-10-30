#include "Tools.h"

#include <cstdio>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <io.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

using namespace winrt::Windows::Data::Json;

namespace {

// The registration manifest is compiled in, so the replies always match what Windows registered.
JsonObject staticResponses() {
    const auto module = GetModuleHandleW(nullptr);
    const auto resource = FindResourceW(module, L"MANIFEST", RT_RCDATA);
    winrt::check_bool(resource != nullptr);
    const std::string_view manifest{ static_cast<char const*>(LockResource(LoadResource(module, resource))), SizeofResource(module, resource) };
    return JsonObject::Parse(winrt::to_hstring(manifest))
        .GetNamedObject(L"_meta")
        .GetNamedObject(L"com.microsoft.windows")
        .GetNamedObject(L"static_responses");
}

void send(JsonObject const& message) {
    const auto line = winrt::to_string(message.Stringify());
    std::fwrite(line.data(), 1, line.size(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

JsonObject envelope(IJsonValue const& id) {
    JsonObject message;
    message.SetNamedValue(L"jsonrpc", JsonValue::CreateStringValue(L"2.0"));
    message.SetNamedValue(L"id", id);
    return message;
}

void reply(IJsonValue const& id, IJsonValue const& result) {
    auto message = envelope(id);
    message.SetNamedValue(L"result", result);
    send(message);
}

void fail(IJsonValue const& id, int code, winrt::hstring const& text) {
    JsonObject error;
    error.SetNamedValue(L"code", JsonValue::CreateNumberValue(code));
    error.SetNamedValue(L"message", JsonValue::CreateStringValue(text));
    auto message = envelope(id);
    message.SetNamedValue(L"error", error);
    send(message);
}

JsonObject toolResult(winrt::hstring const& text, bool isError) {
    JsonObject content;
    content.SetNamedValue(L"type", JsonValue::CreateStringValue(L"text"));
    content.SetNamedValue(L"text", JsonValue::CreateStringValue(text));
    JsonArray contents;
    contents.Append(content);
    JsonObject result;
    result.SetNamedValue(L"content", contents);
    result.SetNamedValue(L"isError", JsonValue::CreateBooleanValue(isError));
    return result;
}

void callTool(IJsonValue const& id, JsonObject const& params) {
    const auto name = params.GetNamedString(L"name", L"");
    const auto arguments = params.HasKey(L"arguments") && params.GetNamedValue(L"arguments").ValueType() == JsonValueType::Object
        ? params.GetNamedObject(L"arguments")
        : JsonObject{};
    try {
        const auto structured = sujud::mcp::call(name, arguments, std::chrono::system_clock::now());
        if (!structured) {
            fail(id, -32602, L"Unknown tool: " + name);
            return;
        }
        auto result = toolResult(structured->Stringify(), false);
        result.SetNamedValue(L"structuredContent", *structured);
        reply(id, result);
    } catch (sujud::mcp::InvalidArguments const& error) {
        reply(id, toolResult(winrt::to_hstring(error.what()), true));
    }
}

}

int wmain() {
    winrt::init_apartment();
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    const auto responses = staticResponses();
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        JsonObject request{ nullptr };
        if (!JsonObject::TryParse(winrt::to_hstring(line), request)) {
            fail(JsonValue::CreateNullValue(), -32700, L"Parse error");
            continue;
        }
        if (!request.HasKey(L"id")) {
            continue;
        }

        const auto id = request.GetNamedValue(L"id");
        const auto method = request.GetNamedString(L"method", L"");
        try {
            if (method == L"initialize" || method == L"tools/list") {
                reply(id, responses.GetNamedObject(method));
            } else if (method == L"ping") {
                reply(id, JsonObject{});
            } else if (method == L"tools/call") {
                callTool(id, request.GetNamedObject(L"params", JsonObject{}));
            } else {
                fail(id, -32601, L"Method not found: " + method);
            }
        } catch (...) {
            fail(id, -32603, winrt::to_message());
        }
    }
    return 0;
}
