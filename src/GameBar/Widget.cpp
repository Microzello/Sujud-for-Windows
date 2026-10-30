#include "pch.h"
#include "Widget.h"

#include "Settings.h"
#include "Strings.h"
#include "sujud/Clock.h"

#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Documents.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>

namespace sujud::gamebar {
namespace {

using namespace std::chrono;
using namespace winrt::Microsoft::Gaming::XboxGameBar;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using winrt::Windows::UI::Color;
using winrt::Windows::UI::Text::FontWeights;
using winrt::Windows::UI::Xaml::Hosting::ElementCompositionPreview;
using winrt::Windows::UI::Xaml::Media::SolidColorBrush;

TextBlock text(double size) {
    TextBlock block;
    block.FontSize(size);
    block.TextTrimming(TextTrimming::CharacterEllipsis);
    Documents::Typography::SetNumeralAlignment(block, FontNumeralAlignment::Tabular);
    return block;
}

constexpr winrt::Windows::Foundation::Size defaultSize{ 280, 40 };
constexpr winrt::Windows::Foundation::Size maximumSize{ 480, 120 };

std::wstring timeOf(Instant instant) {
    return clock::shortTime(clock::local(instant));
}

}

UIElement Widget::shadowed(TextBlock const& block) {
    Grid cell;
    Canvas host;
    cell.Children().Append(host);
    cell.Children().Append(block);

    const auto compositor = ElementCompositionPreview::GetElementVisual(cell).Compositor();
    shadow = compositor.CreateDropShadow();
    shadow.BlurRadius(6);
    shadow.Offset({ 0, 1, 0 });
    shadow.Mask(block.GetAlphaMask());
    auto sprite = compositor.CreateSpriteVisual();
    sprite.Shadow(shadow);
    ElementCompositionPreview::SetElementChildVisual(host, sprite);
    block.SizeChanged([sprite](auto&&, SizeChangedEventArgs const& e) {
        sprite.Size({ static_cast<float>(e.NewSize().Width), static_cast<float>(e.NewSize().Height) });
    });
    return cell;
}

UIElement Widget::build() {
    line = text(16);
    title.FontWeight(FontWeights::SemiBold());
    line.Inlines().Append(title);
    line.Inlines().Append(at);
    Viewbox fit;
    fit.StretchDirection(StretchDirection::DownOnly);
    fit.HorizontalAlignment(HorizontalAlignment::Left);
    fit.Child(shadowed(line));
    ready = fit;

    unset.Content(winrt::box_value(winrt::hstring{ strings::get(L"SetLocation") }));
    unset.Padding({ 0 });
    unset.Click([](auto&&, auto&&) { winrt::Windows::System::Launcher::LaunchUriAsync(winrt::Windows::Foundation::Uri{ L"sujud:" }); });

    root.Padding({ 10, 2, 10, 2 });
    root.VerticalAlignment(VerticalAlignment::Center);
    root.FlowDirection(strings::isRightToLeft() ? FlowDirection::RightToLeft : FlowDirection::LeftToRight);
    root.Children().Append(ready);
    root.Children().Append(unset);
    return root;
}

void Widget::start(XboxGameBarWidgetActivatedEventArgs const& args) {
    opened = floor<seconds>(system_clock::now());

    Frame frame;
    frame.Content(build());
    Window::Current().Content(frame);
    widget = XboxGameBarWidget(args, Window::Current().CoreWindow(), frame);
    notifications = XboxGameBarWidgetNotificationManager(widget);

    themeChanged = widget.RequestedThemeChanged(winrt::auto_revoke, { get_weak(), &Widget::onThemeChanged });
    applyTheme();

    timer.Interval(1s);
    timer.Tick([weak = get_weak()](auto&&, auto&&) {
        if (const auto self = weak.get()) {
            self->refresh();
        }
    });
    refresh();
    timer.Start();
    Window::Current().Activate();
    fitWindow();
}

// Game Bar restores the window size a widget had in earlier versions, even past
// today's maximum, and times the widget out if it resizes during activation.
winrt::fire_and_forget Widget::fitWindow() {
    const auto strong = get_strong();
    co_await winrt::resume_background();
    try {
        const auto bounds = widget.WindowBounds();
        if (bounds.Width > maximumSize.Width || bounds.Height > maximumSize.Height) {
            co_await widget.TryResizeWindowAsync(defaultSize);
        }
    } catch (winrt::hresult_error const&) {
    }
}

void Widget::stop() {
    timer.Stop();
    themeChanged.revoke();
    notifications = nullptr;
    widget = nullptr;
}

winrt::fire_and_forget Widget::onThemeChanged(winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IInspectable) {
    const auto strong = get_strong();
    co_await winrt::resume_foreground(root.Dispatcher());
    if (widget) {
        applyTheme();
    }
}

void Widget::applyTheme() {
    const auto theme = widget.RequestedTheme();
    const bool light = theme == ElementTheme::Light;
    root.RequestedTheme(theme);
    line.Foreground(SolidColorBrush{ light ? Color{ 255, 26, 26, 26 } : Color{ 255, 255, 255, 255 } });
    shadow.Color(light ? Color{ 255, 255, 255, 255 } : Color{ 255, 0, 0, 0 });
    shadow.Opacity(0.9f);
}

void Widget::refresh() {
    const auto now = system_clock::now();
    const auto instant = floor<seconds>(now);
    const auto timetable = settings::timetable();
    const auto status = timetable ? timetable->status(instant) : Status{};
    const bool isReady = timetable && status.next;

    ready.Visibility(isReady ? Visibility::Visible : Visibility::Collapsed);
    unset.Visibility(isReady ? Visibility::Collapsed : Visibility::Visible);
    if (!isReady) {
        return;
    }

    notifyIfDue(status, instant);

    const auto& next = *status.next;
    title.Text(strings::format(L"NextPrayerIn", { strings::prayer(next.prayer), clock::countdown(now, next.time) }));
    at.Text(L" " + strings::format(L"AtTime", { timeOf(next.time) }));
}

void Widget::notifyIfDue(Status const& status, Instant now) {
    if (!status.current || !isPrayer(status.current->prayer)) {
        return;
    }
    const auto& current = *status.current;
    if (current.time <= opened || current.time == notified || now - current.time > 2min) {
        return;
    }
    notified = current.time;
    notify(current);
}

winrt::fire_and_forget Widget::notify(Event event) {
    const auto strong = get_strong();
    try {
        if (!notifications || notifications.Setting() != XboxGameBarWidgetNotificationSetting::Enabled) {
            co_return;
        }
        const auto notification = XboxGameBarWidgetNotificationBuilder(winrt::hstring{ strings::format(L"PrayerTimeNotification", { strings::prayer(event.prayer) }) })
            .Content(winrt::hstring{ timeOf(event.time) })
            .PrimaryImageFromPublicFolder(L"Icons\\icon.targetsize-256.png", XboxGameBarWidgetNotificationImageCrop::Default)
            .IgnoreQuietHours(true)
            .BuildNotification();
        co_await notifications.TryShowAsync(notification);
    } catch (winrt::hresult_error const&) {
    }
}

}
