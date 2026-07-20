# ClickyWeather

Focused weather cards for button-first Pebbles.

ClickyWeather is a card-based weather app for Pebble built around quick button presses and glanceable forecast data.

## Install

Download a tagged PBW from [GitHub Releases](https://github.com/tylxr59/ClickyWeather/releases).

## Overview

ClickyWeather is a fork of [TouchyWeather](https://github.com/ClickCalickClick/TouchyWeather) by ClickCalickClick,
reimagined around button-based navigation. If you prefer navigating with hardware buttons over touch, this one's for you.

ClickyWeather is built around fast card navigation, button-triggered refresh, and focused cards that keep the watch UI lean.

### Changes from TouchyWeather

- **Navigation animations** — Reworked to reflect vertical up/down button navigation instead of horizontal swipe transitions
- **Radar and Touch & Go card removed** — Dropped in favor of keeping the app lean and card-focused
- **Settings moved** — Relocated from an in-app card to the Pebble companion app for a cleaner card flow

## Features

ClickyWeather is a carousel of focused weather cards:

- **Main** — current temp, feels-like, high/low, wind, humidity, quick status
- **6 Hours** — next 6-hour outlook
- **Week Ahead** — 5-day forecast snapshot
- **Precipitation** — near-term rain probability view
- **UV** — UV index + risk context
- **Air Quality** — AQI + air quality signal
- **Sun Cycle** — sunrise/sunset timing
- **Night Sky** — moon phase + illumination
- **Golden Hour** — blue/golden hour timing blocks
- **Alerts** — US weather alerts from the National Weather Service, with "NO DATA" outside supported regions
- **Background refresh** — optional updates from every 30 minutes to every 24 hours while the app is closed
- **App update checks** — choose Never, every weather refresh, a timed interval, or an immediate manual check
- **Time formats** — match the watch clock or force 12/24-hour weather times
- **Detailed forecasts** — hold SELECT on 6 Hours, Week Ahead, Precipitation, UV, or Air Quality for a deeper view
- **Release notices** — the watch flags newer GitHub releases and shows their notes once after installation

## Privacy

Privacy is a core product requirement. ClickyWeather does not include analytics,
telemetry, advertising identifiers, usage tracking, or user profiling. It sends
location coordinates only to the weather and alert services needed to produce
the forecast. Optional update checks contact only the public GitHub Releases API
at the configured frequency (daily by default) or when requested manually; they
do not include location or an app-added device identifier. No upstream analytics
or tracking changes will be accepted.

## Screenshots

Current checked-in screenshots are for **emery**. Use UP/DOWN to move between cards.

<table>
  <tr>
    <th>Main</th>
    <th>6 Hours</th>
    <th>Week Ahead</th>
  </tr>
  <tr>
    <td><img src="screenshots/emery%20-%20main.png" width="144"></td>
    <td><img src="screenshots/emery%20-%206%20hour.png" width="144"></td>
    <td><img src="screenshots/emery%20-%20week%20ahead.png" width="144"></td>
  </tr>
  <tr>
    <th>Precipitation</th>
    <th>Alerts</th>
  </tr>
  <tr>
    <td><img src="screenshots/emery%20-%20precipitation.png" width="144"></td>
    <td><img src="screenshots/emery%20-%20weather%20alerts.png" width="144"></td>
  </tr>
</table>

## Setup

ClickyWeather lets you customize its display, weather refresh, and app update checks through the **phone app settings**.

Under **App Updates**, choose how often the paired phone may check GitHub for a
new release. Timed checks happen when the app opens or weather refreshes and do
not create a separate watch wakeup. **Every weather refresh** can increase phone
network use when background weather updates are frequent; **Never automatically**
disables scheduled checks. **Check for updates now** saves and closes settings,
then performs one immediate check regardless of the selected interval.

Open the phone app's configuration page and navigate to the **Cards** section. Toggle any of the 9 optional cards on or off to build your ideal weather deck:

- **6 Hours**, **Week Ahead**, **Precipitation**, **UV Index**, **Air Quality**, **Sun Cycle**, **Night Sky**, **Golden Hour**, **Alerts**

Your choices sync to the watch immediately. This means you can run ultra-minimal (just core weather) or full nerd mode (everything enabled).

The **Main** card is always enabled and cannot be disabled—it is the anchor weather view.

## Controls

- **UP/DOWN**: previous/next card
- **SELECT**: refresh weather
- **Hold SELECT**: open details on supported forecast cards
- **BACK**: close a detail or release-notes screen; otherwise exit

## Requirements

ClickyWeather targets only:

- Pebble Time 2 / `emery`
- Pebble Round 2 / `gabbro`

- A paired phone with location access and internet connectivity for weather updates
- Pebble SDK / Pebble Tool, only when building locally

No legacy Pebble targets are included.

## Troubleshooting

- If weather does not load, confirm that the paired phone has internet and location access.
- If alerts show `NO DATA`, the configured location may be outside the US National Weather Service coverage area.
- If card changes do not appear, reopen settings and save the card selection again.

## Development

```bash
npm ci
npm run build
```

Install to emulator (headless-friendly):

```bash
npm run install:emery
```

Take a screenshot:

```bash
pebble screenshot --emulator emery --vnc --no-open screenshot.png
```

The compiled package is written to `build/ClickyWeather.pbw`.

## Releases

Every release has a newest-first entry in `CHANGELOG.md`. Its bullets appear once
on the watch after the update and are also used as the GitHub Release notes.

Pushing a version tag such as `v1.3.1` runs the GitHub Actions release workflow. The workflow verifies that the tag matches `CHANGELOG.md`, `package.json`, and `src/pkjs/version.js`, builds the PBW, uploads it as a workflow artifact, and attaches it to the matching GitHub Release. Because ClickyWeather is distributed outside the Pebble Appstore, configurable phone-side checks can query GitHub Releases and display `UPDATE AVAILABLE` when a newer tagged PBW can be downloaded.

To repair a missing or outdated PBW for an existing tag, open **Actions → Release PBW → Run workflow** and enter that tag. Manual runs check out the exact tag before rebuilding and replacing the release asset. Release current code with a new version and tag instead of reusing an older tag.

## Project Layout

```text
src/c/                 Pebble C app, weather cards, navigation, and UI
src/pkjs/              PebbleKitJS weather requests, settings, and version check
CHANGELOG.md            Release notes used by the watch and GitHub Releases
resources/images/      Pebble menu icon
screenshots/           Checked-in Pebble Time 2 screenshots
package.json           Pebble metadata, scripts, and message keys
wscript                Pebble SDK build script
pebble-appstore.md     Version-controlled proposed Pebble Appstore listing copy
```

## License

ClickyWeather is a modified version of TouchyWeather by ClickCalickClick.

This fork is licensed under the same Source-Available Hybrid License
(CC BY-NC 4.0 with Marketplace Restrictions). Original work is copyright
ClickCalickClick; ClickyWeather modifications are copyright tylxr.

Please see the full [LICENSE.md](LICENSE.md) file for details regarding permissions, non-commercial use, and app store distribution rules.

## Notes

AI assistance was used during development for code review, UI polish, documentation drafting, and implementation support. The app behavior, configuration choices, and release decisions were reviewed by the project maintainer.
