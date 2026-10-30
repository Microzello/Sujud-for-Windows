# Sujud for Windows: Microsoft Store listing

Copy these into Partner Center (Apps and games › Sujud › Start submission).

## Pricing and availability

Free, in all markets, public and discoverable.

## Properties

| Field | Value |
|---|---|
| Category | Lifestyle |
| Privacy policy URL | https://github.com/Microzello/Sujud-for-Windows/blob/main/Store/PrivacyPolicy.md |
| Website | https://github.com/Microzello/Sujud-for-Windows |
| Support contact info | omar@omarconsultancy.com |

## Age ratings

Choose a non-game app and answer "No" to every content question, so it gets the lowest rating.

## Packages

Upload `AppPackages\Sujud_1.0.7.0_x64_arm64.msixupload` (built locally, not in the repo). Leave only the Windows Desktop device family selected.

## Submission options

**Restricted capability (runFullTrust)**

> Sujud is a desktop app built on the Windows App SDK. It needs full trust to run a widget provider for Windows widgets, a background task that keeps prayer notifications scheduled, and an MCP server that Windows registers as an agent connector.

**Notes for certification**

> Sujud has no account. On first launch it asks for a location: click Current Location to use the PC's location, or type a latitude and longitude (for example 21.4225 and 39.8262) and click Set Location. The Location button at the top changes it later.
>
> • Widgets: press Win+W, choose Add widgets, then Sujud. Next Prayer comes in small and medium, Prayer Times in small, medium and large. The small sizes can be added to the lock screen under Settings › Personalization › Lock screen › Your widgets.
> • The calculation method and Asr convention are in Settings (gear icon).
> • On PCs with a compass, the Qibla dial follows the device. Reverse compass corrects PCs whose sensor reports the heading the wrong way round.
> • A notification arrives at each prayer time.
> • Full screen: the button beside Qibla covers the screen with the countdown to the next prayer and keeps the display on. Any key or click closes it. Settings › Full Screen › Show prayer times adds today's times beside it.
> • Xbox Game Bar: press Win+G, open the Widget menu and choose Sujud. It shows the location set in the app and can be pinned over a game. At each prayer time it shows a Game Bar notification.
> • Agent connector: the package registers an MCP server (Sujud.Mcp.exe) with Windows for AI agents. It has no window and runs only when an agent starts it.
> • The package declares three applications: Sujud itself, the MCP server and the Game Bar widget (a UWP app, as Game Bar requires). Only Sujud appears in Start.

## Store listing: English (United States)

**Product name**: Sujud

**Short description**

> Prayer times for wherever you are, with widgets for the Widgets board and the lock screen. Offline, private, and nothing extra.

**Description**

> Prayer times for where you are, and nothing else.
>
> Sujud shows today's six times (Fajr, Sunrise, Dhuhr, Asr, Maghrib and Isha) with a live countdown to the next prayer. Open it full screen to keep the countdown on a spare display, with the display kept awake.
>
> Pin the Sujud widget in Xbox Game Bar to keep the countdown over your game, with a Game Bar notification at each prayer time. AI agents on Windows can ask Sujud for prayer times and the Qibla direction too.
>
> Use your PC's location, or enter the coordinates of any place. Every time is calculated on your PC, even offline. There are no accounts, ads, or tracking, and your location stays on your PC.
>
> Choose a calculation method in Settings. Muslim World League is the default. ISNA, Egypt, Makkah (Umm al-Qura), Karachi, Tehran, Jafari, France, Russia and Singapore are also available, each with Standard or Hanafi Asr.

**Product features** (one per line, no bullets)

```
Today's six prayer times with a live countdown to the next
Next Prayer and Prayer Times widgets for the Widgets board
Small widgets for the lock screen
A notification at each prayer time
A full screen countdown that keeps the display on
An Xbox Game Bar widget you can pin over games
Qibla direction, with a live compass on PCs that have one
An agent connector for AI agents on Windows
Ten calculation methods with Standard or Hanafi Asr
Calculated on your PC and works offline
No accounts, ads or tracking
Follows your Windows language and 12- or 24-hour time
English, Arabic, Bengali, French, German, Indonesian, Malay, Persian, Russian, Spanish, Turkish and Urdu
```

If the form shows these fields:

- **Search terms**: prayer times, salah, namaz, qibla, adhan, ramadan, islamic
- **Copyright**: 2026 Omar Ahmed

## Images

In `Screenshots/`, as 1920 × 1080 PNGs. Upload them under Desktop in this order, with these captions:

| File | Caption |
|---|---|
| `Desktop - 1 Prayer Times.png` | Today's prayer times, counting down to the next. |
| `Desktop - 2 Qibla.png` | The Qibla direction, on a live compass if your PC has one. |
| `Desktop - 3 Widgets.png` | Next Prayer and Prayer Times widgets for the Widgets board, in every size. |
| `Desktop - 4 Settings.png` | Choose the calculation method and Asr convention. |

Upload `App tile icon 300.png` as the 1:1 app tile icon (300 × 300) under Store logos.

The app screenshots show Toronto. The widgets were captured a minute or two apart, so their countdowns differ slightly.
