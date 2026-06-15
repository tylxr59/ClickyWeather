# ClickyWeather
**Focused weather cards for button-first Pebbles.**

ClickyWeather is a card-based weather app for Pebble built around quick button presses and glanceable forecast data.

## 🌟 Why this app is different
### **Focused weather UX** on Pebble  
### **Built only for `gabbro` and `emery`**  

ClickyWeather is a fork of [TouchyWeather](https://github.com/ClickCalickClick/TouchyWeather) by ClickCalickClick,
reimagined around button-based navigation. If you prefer navigating with hardware buttons over touch, this one's for you.

Most Pebble weather apps are bare-bones. ClickyWeather is built around fast card navigation, button-triggered refresh,
and focused cards that keep the watch UI lean.

### Changes from TouchyWeather

- **Navigation animations** — Reworked to reflect vertical up/down button navigation instead of horizontal swipe transitions
- **Radar and Touch & Go card removed** — Dropped in favor of keeping the app lean and card-focused
- **Settings moved** — Relocated from an in-app card to the Pebble companion app for a cleaner card flow

---

## Feature Overview

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

---

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

---

## Customizing Your Cards

ClickyWeather lets you customize which cards appear on your watch through the **phone app settings**.

Open the phone app's configuration page and navigate to the **Cards** section. Toggle any of the 9 optional cards on or off to build your ideal weather deck:

- **6 Hours**, **Week Ahead**, **Precipitation**, **UV Index**, **Air Quality**, **Sun Cycle**, **Night Sky**, **Golden Hour**, **Alerts**

Your choices sync to the watch immediately. This means you can run ultra-minimal (just core weather) or full nerd mode (everything enabled).

**Note:** The **Main** card is always enabled and cannot be disabled—it's your anchor weather view.

---

## Controls

- **UP/DOWN**: previous/next card
- **SELECT**: refresh weather

---

## Platform Support

ClickyWeather targets only:

- `emery`
- `gabbro`

No legacy non-touch Pebble targets are included.

---

## Build / Run

```bash
pebble build
```

Install to emulator (headless-friendly):

```bash
pebble install --emulator emery --vnc
```

Take a screenshot:

```bash
pebble screenshot --emulator emery --vnc --no-open screenshot.png
```

---

## License

This project is licensed under a Source-Available Hybrid License (CC BY-NC 4.0 with Marketplace Restrictions).

Please see the full [LICENSE.md](LICENSE.md) file for details regarding permissions, non-commercial use, and app store distribution rules.
