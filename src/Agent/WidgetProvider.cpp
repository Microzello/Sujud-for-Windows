#include "pch.h"
#include "WidgetProvider.h"

#include "Notifications.h"
#include "WidgetContent.h"

#include <map>
#include <mutex>

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.h>

namespace sujud {
namespace {

using namespace std::chrono;
using namespace winrt::Microsoft::Windows::Widgets::Providers;

template <typename Work>
void quietly(Work&& work) noexcept {
    try {
        work();
    } catch (...) {
        LOG_CAUGHT_EXCEPTION();
    }
}

// The widget host may create several providers; they share the pinned widgets and one timer.
class Board {
public:
    static Board& shared() {
        static Board board;
        return board;
    }

    void attach() {
        std::scoped_lock lock{ mutex };
        if (providers++ == 0) {
            quietly([&] {
                for (auto const& info : WidgetManager::GetDefault().GetWidgetInfos()) {
                    const auto context = info.WidgetContext();
                    widgets.insert_or_assign(std::wstring{ context.Id() }, std::wstring{ context.DefinitionId() });
                }
            });
            schedule();
        }
    }

    void detach() {
        std::scoped_lock lock{ mutex };
        if (--providers == 0) {
            timer.reset();
        }
    }

    void show(WidgetContext const& context) {
        std::wstring id{ context.Id() };
        std::wstring definition{ context.DefinitionId() };
        {
            std::scoped_lock lock{ mutex };
            widgets.insert_or_assign(id, definition);
        }
        quietly([&] { widgets::push(id, definition); });
    }

    void remove(std::wstring const& id) {
        std::scoped_lock lock{ mutex };
        widgets.erase(id);
    }

private:
    void refreshAll() {
        std::map<std::wstring, std::wstring> pinned;
        {
            std::scoped_lock lock{ mutex };
            pinned = widgets;
        }
        for (auto const& [id, definition] : pinned) {
            quietly([&] { widgets::push(id, definition); });
        }
        std::scoped_lock lock{ mutex };
        if (providers > 0) {
            schedule();
        }
    }

    void schedule() {
        if (!timer) {
            timer.reset(CreateThreadpoolTimer(
                [](PTP_CALLBACK_INSTANCE, void* context, PTP_TIMER) { static_cast<Board*>(context)->refreshAll(); },
                this, nullptr));
        }
        const auto due = widgets::nextRefresh(system_clock::now()) + 250ms;
        auto file = winrt::clock::to_file_time(winrt::clock::from_sys(due));
        SetThreadpoolTimer(timer.get(), reinterpret_cast<FILETIME*>(&file), 0, 0);
    }

    std::mutex mutex;
    std::map<std::wstring, std::wstring> widgets;
    int providers = 0;
    wil::unique_threadpool_timer_nowait timer;
};

}

WidgetProvider::WidgetProvider() {
    Board::shared().attach();
}

WidgetProvider::~WidgetProvider() {
    Board::shared().detach();
}

void WidgetProvider::CreateWidget(WidgetContext const& context) {
    Board::shared().show(context);
    quietly([] { notifications::reschedule(); });
}

void WidgetProvider::DeleteWidget(winrt::hstring const& widgetId, winrt::hstring const&) {
    Board::shared().remove(std::wstring{ widgetId });
}

void WidgetProvider::OnActionInvoked(WidgetActionInvokedArgs const& args) {
    if (args.Verb() != L"open") {
        return;
    }
    quietly([] {
        const auto entries = winrt::Windows::ApplicationModel::Package::Current().GetAppListEntries();
        if (entries.Size() > 0) {
            entries.GetAt(0).LaunchAsync();
        }
    });
}

void WidgetProvider::OnWidgetContextChanged(WidgetContextChangedArgs const& args) {
    Board::shared().show(args.WidgetContext());
}

void WidgetProvider::Activate(WidgetContext const& context) {
    Board::shared().show(context);
    quietly([] { notifications::reschedule(); });
}

void WidgetProvider::Deactivate(winrt::hstring const&) {
}

}
