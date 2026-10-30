#include "pch.h"
#include "FullScreen.h"

#include "Settings.h"
#include "Strings.h"
#include "sujud/Clock.h"

#include <array>
#include <memory>
#include <optional>

#include <winrt/Microsoft.UI.Xaml.Documents.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Core.h>

namespace sujud::fullscreen {
namespace {

using namespace std::chrono;
using namespace winrt::Microsoft::UI::Windowing;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using winrt::Microsoft::UI::Text::FontWeights;
using winrt::Microsoft::UI::Xaml::Media::SolidColorBrush;
using winrt::Microsoft::UI::Xaml::Media::TranslateTransform;
using winrt::Windows::UI::Color;
using winrt::Windows::UI::Text::FontWeight;

// Apple's text on black: #F5F5F7, and the iOS secondary label, #EBEBF5 at 60%.
constexpr Color primary{ 255, 0xF5, 0xF5, 0xF7 };
constexpr Color secondary{ 153, 0xEB, 0xEB, 0xF5 };

constexpr double share = 0.5;

TextBlock label(double size, FontWeight weight, Color color) {
    TextBlock block;
    block.FontSize(size);
    block.FontWeight(weight);
    block.Foreground(SolidColorBrush{ color });
    Documents::Typography::SetNumeralAlignment(block, FontNumeralAlignment::Tabular);
    return block;
}

std::wstring timeOf(Instant instant) {
    return clock::shortTime(clock::local(instant));
}

GridLength automatic() {
    return GridLengthHelper::Auto();
}

class View {
public:
    explicit View(winrt::Microsoft::UI::WindowId owner);
    ~View();

private:
    struct Row {
        TextBlock name{ nullptr };
        TextBlock time{ nullptr };
    };

    void build();
    void arrange(winrt::Windows::Foundation::Size size);
    void center();
    void tick();
    void scheduleTick();
    void close();

    Window window;
    ContentControl root;
    Viewbox fit;
    Grid layout;
    StackPanel next;
    TranslateTransform shift;
    TextBlock name = label(28, FontWeights::SemiBold(), primary);
    TextBlock in = label(20, FontWeights::SemiBold(), primary);
    TextBlock at = label(14, FontWeights::Normal(), secondary);
    Grid table;
    std::array<Row, prayers.size()> rows;

    std::optional<Timetable> timetable = settings::timetable();
    bool showTimes = settings::fullScreenTimes();
    bool portrait = false;
    winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer timer{ nullptr };
    wil::unique_handle awake;
    bool active = false;
    bool closing = false;
};

std::unique_ptr<View> current;

View::View(winrt::Microsoft::UI::WindowId owner) {
    build();

    const auto appWindow = window.AppWindow();
    appWindow.Title(L"Sujud");
    if (const auto display = DisplayArea::GetFromWindowId(owner, DisplayAreaFallback::Nearest)) {
        appWindow.MoveAndResize(display.WorkArea());
    }
    appWindow.SetPresenter(AppWindowPresenterKind::FullScreen);

    REASON_CONTEXT reason{ POWER_REQUEST_CONTEXT_VERSION, POWER_REQUEST_CONTEXT_SIMPLE_STRING };
    reason.Reason.SimpleReasonString = const_cast<LPWSTR>(L"Showing the next prayer full screen");
    if (const auto request = PowerCreateRequest(&reason); request != INVALID_HANDLE_VALUE) {
        awake.reset(request);
        PowerSetRequest(request, PowerRequestDisplayRequired);
        PowerSetRequest(request, PowerRequestSystemRequired);
    }

    root.SizeChanged([this](auto&&, SizeChangedEventArgs const& args) { arrange(args.NewSize()); });
    table.SizeChanged([this](auto&&, auto&&) { center(); });
    root.Loaded([this](auto&&, auto&&) { root.Focus(FocusState::Programmatic); });
    root.KeyDown([this](auto&&, Input::KeyRoutedEventArgs const& args) {
        if (!args.KeyStatus().WasKeyDown) {
            close();
        }
    });
    root.PointerPressed([this](auto&&, auto&&) { close(); });
    window.Activated([this](auto&&, WindowActivatedEventArgs const& args) {
        if (args.WindowActivationState() == WindowActivationState::Deactivated) {
            if (active) {
                close();
            }
            return;
        }
        active = true;
        root.Focus(FocusState::Programmatic);
    });
    window.Closed([](auto&&, auto&&) {
        winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread().TryEnqueue([] { current.reset(); });
    });

    timer = window.DispatcherQueue().CreateTimer();
    timer.IsRepeating(false);
    timer.Tick([this](auto&&, auto&&) {
        tick();
        scheduleTick();
    });
    tick();
    scheduleTick();
    window.Activate();
}

View::~View() {
    timer.Stop();
}

void View::build() {
    next.VerticalAlignment(VerticalAlignment::Center);
    next.RenderTransform(shift);
    at.Margin({ 0, 4, 0, 0 });
    for (auto const& line : { name, in, at }) {
        line.HorizontalAlignment(HorizontalAlignment::Center);
        next.Children().Append(line);
    }

    for (int index = 0; index < 2; ++index) {
        RowDefinition row;
        row.Height(automatic());
        layout.RowDefinitions().Append(row);
        ColumnDefinition column;
        column.Width(automatic());
        layout.ColumnDefinitions().Append(column);
    }

    table.VerticalAlignment(VerticalAlignment::Center);
    table.ColumnSpacing(24);
    table.Opacity(showTimes ? 1 : 0);
    table.ColumnDefinitions().Append(ColumnDefinition{});
    ColumnDefinition timeColumn;
    timeColumn.Width(automatic());
    table.ColumnDefinitions().Append(timeColumn);
    for (const auto prayer : prayers) {
        const auto index = static_cast<int32_t>(prayer);
        RowDefinition definition;
        definition.Height(automatic());
        table.RowDefinitions().Append(definition);

        auto& row = rows[static_cast<std::size_t>(prayer)];
        row.name = label(14, FontWeights::Normal(), primary);
        row.name.Text(strings::prayer(prayer));
        row.time = label(14, FontWeights::Normal(), primary);
        row.time.HorizontalAlignment(HorizontalAlignment::Right);
        Grid::SetRow(row.name, index);
        Grid::SetRow(row.time, index);
        Grid::SetColumn(row.time, 1);
        table.Children().Append(row.name);
        table.Children().Append(row.time);
    }

    layout.Children().Append(next);
    layout.Children().Append(table);

    fit.HorizontalAlignment(HorizontalAlignment::Center);
    fit.VerticalAlignment(VerticalAlignment::Center);
    fit.Child(layout);

    Grid surface;
    surface.Background(SolidColorBrush{ Color{ 255, 0, 0, 0 } });
    surface.Children().Append(fit);

    root.IsTabStop(true);
    root.UseSystemFocusVisuals(false);
    root.HorizontalContentAlignment(HorizontalAlignment::Stretch);
    root.VerticalContentAlignment(VerticalAlignment::Stretch);
    root.FlowDirection(strings::isRightToLeft() ? FlowDirection::RightToLeft : FlowDirection::LeftToRight);
    root.Content(surface);
    window.Content(root);
}

void View::arrange(winrt::Windows::Foundation::Size size) {
    fit.Width(size.Width * share);
    fit.MaxHeight(size.Height * 0.9);

    portrait = size.Height > size.Width;
    Grid::SetRow(table, portrait ? 1 : 0);
    Grid::SetColumn(table, portrait ? 0 : 1);
    layout.RowSpacing(portrait ? 32 : 0);
    layout.ColumnSpacing(portrait ? 0 : 40);
    table.RowSpacing(portrait ? 6 : 2);
    table.HorizontalAlignment(portrait ? HorizontalAlignment::Stretch : HorizontalAlignment::Center);
    for (auto const& row : rows) {
        row.name.FontSize(portrait ? 20 : 14);
        row.time.FontSize(portrait ? 20 : 14);
    }
    center();
}

void View::center() {
    if (showTimes) {
        return;
    }
    shift.X(portrait ? 0 : (table.ActualWidth() + layout.ColumnSpacing()) / 2);
    shift.Y(portrait ? (table.ActualHeight() + layout.RowSpacing()) / 2 : 0);
}

void View::scheduleTick() {
    const auto now = system_clock::now();
    timer.Interval(duration_cast<winrt::Windows::Foundation::TimeSpan>(ceil<seconds>(now + 1ms) - now));
    timer.Start();
}

void View::tick() {
    const auto now = system_clock::now();
    const auto instant = floor<seconds>(now);
    const auto status = timetable ? timetable->status(instant) : Status{};
    fit.Visibility(status.next ? Visibility::Visible : Visibility::Collapsed);
    if (!status.next) {
        return;
    }

    const auto& upcomingPrayer = *status.next;
    name.Text(strings::prayer(upcomingPrayer.prayer));
    in.Text(strings::format(L"InDuration", { clock::countdown(now, upcomingPrayer.time) }));
    at.Text(strings::format(L"AtTime", { timeOf(upcomingPrayer.time) }));

    const auto upcoming = timetable->upcoming(instant);
    for (const auto prayer : prayers) {
        const auto index = static_cast<std::size_t>(prayer);
        auto const& row = rows[index];
        row.time.Text(upcoming[index] ? timeOf(upcoming[index]->time) : L"-");
        const auto weight = prayer == upcomingPrayer.prayer ? FontWeights::SemiBold() : FontWeights::Normal();
        row.name.FontWeight(weight);
        row.time.FontWeight(weight);
    }
}

void View::close() {
    if (closing) {
        return;
    }
    closing = true;
    timer.Stop();
    window.Close();
}

}

void show(winrt::Microsoft::UI::WindowId owner) {
    if (!current) {
        current = std::make_unique<View>(owner);
    }
}

}
