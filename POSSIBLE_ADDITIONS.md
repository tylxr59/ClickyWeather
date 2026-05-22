# Possible Weather Data Additions

This document outlines potential enhancements to ClickyWeather based on available data from Open-Meteo and other sources. Most items marked as "No extra API call" are already being fetched but not yet displayed.

---

## Low-Hanging Fruit (Already in Open-Meteo Response)

### 1. 🌬️ Wind Gusts vs. Sustained Wind
**Status:** Available in current API, not fetched  
**Impact:** High — gusts are the real threat (actual knockdown force)  
**Details:**
- Currently: only `wind_speed_10m` (10-minute sustained average)
- Available: `wind_gusts_10m` in the `current` response
- Use case: "Sustained 15 mph, gusts to 28 mph" is way more useful than "15 mph"

**Implementation:**
- Add `wind_gust` field to `WeatherData` struct
- Fetch `wind_gusts_10m` in PKJS forecast request
- Update main card display to use it

---

### 2. 🌡️ Apparent Temperature in Hourly & Daily Cards
**Status:** Available in current API, not fetched for forecast  
**Impact:** Medium — helps users understand near-term comfort  
**Details:**
- Currently: Only shown on Main card as "FEELS LIKE"
- Available: `hourly.apparent_temperature` and `daily.apparent_temperature_2m_max/min`
- Use case: A 6-hour forecast showing "72°F but feels like 82°F" changes planning
- Note: Already computed and sent for current conditions; just not for future hours/days

**Implementation:**
- Add `hours_feels_like[6]` and `days_feels_like[4]` to `WeatherData`
- Fetch `apparent_temperature` in hourly and daily requests
- Display alongside regular temperature in the Hours and Week Ahead cards

---

### 3. 🌧️ Precipitation Amount (mm/inches) vs. Just Probability
**Status:** Available in current API, not fetched  
**Impact:** High — probability alone is incomplete information  
**Details:**
- Currently: Precipitation card shows probability bars (0–100%)
- Available: `hourly.precipitation` (mm) in the same request
- Gap: A 90% chance of 0.2mm feels different than 90% chance of 20mm
- Use case: Users need to know "light sprinkle" vs. "downpour" to pack umbrellas or cancel plans

**Implementation:**
- Add `precip_amount[5]` (mm) to `WeatherData`
- Fetch `hourly.precipitation` alongside `precipitation_probability`
- Display amount on precipitation card (e.g., "95% · 12mm" or scale bars by amount, not just probability)

---

### 4. 🌍 Wind Data in Hourly & Weekly Cards
**Status:** Partially available (hourly wind is fetched but not used)  
**Impact:** Medium — helps with activity planning  
**Details:**
- Currently: Wind shown only on Main card (current conditions)
- Available: `hourly.wind_speed_10m` and `hourly.wind_direction_10m` are already fetched but not stored
- Gap: Planning an outdoor event in 3 hours? No way to see if it'll be windy
- Note: Week Ahead doesn't provide hourly granularity in Open-Meteo, but daily max wind is available

**Implementation:**
- Add `hours_wind_speed[6]` and `hours_wind_dir[6]` to `WeatherData`
- Store already-fetched hourly wind data
- Display wind speed + direction on Hours card

---

### 5. 🕐 Extended Hourly Forecast (12–24 Hours)
**Status:** Already fetched, limited to +6h display  
**Impact:** Medium — common competitor feature  
**Details:**
- Currently: "6 Hours" card shows +1h through +6h only
- Available: Open-Meteo returns hourly data for the entire forecast_days (4 days = 96 hours)
- Cost: Zero — data is already being downloaded
- Gap: Most weather apps show 12 or 24 hours of hourly detail; 6 is minimal

**Implementation:**
- Decide on display scope (12 or 24 hours)
- Extend `WeatherData` hourly arrays from [6] to [12] or [24]
- Update Hours card rendering (may require scrollable/paginated list or condensed layout)

---

### 6. 🌫️ Visibility (meters / miles)
**Status:** Available in current API, not fetched  
**Impact:** Low–Medium (useful for specific use cases: flying, driving in fog)  
**Details:**
- Currently: Fog condition icon exists but no visibility number
- Available: `current.visibility` (meters)
- Use case: "Fog" is vague; "Visibility 200m" tells you whether to cancel the flight or just drive carefully
- Rarity: Mostly relevant for fog/heavy rain; useless on clear days

**Implementation:**
- Add `visibility` field to `WeatherData` (in meters; convert to miles for imperial users)
- Fetch `visibility` in current request
- Display on Main card (e.g., as a detail badge) or create a dedicated visibility mini-card
- Only show when visibility < 1km (5 km+) is usually just labeled "excellent" or omitted

---

## Moderate Effort (Requires New Data Source)

### 7. 🌍 Pollen Outside Europe
**Status:** Partially implemented (code exists but is never called)  
**Impact:** Medium–High (critical for allergy sufferers in pollen season)  
**Details:**
- Currently: Europe only (lat 35–72°N, lon 25°W–45°E) via Open-Meteo CAMS
- Gap: North America, Asia, Australia, Southern Hemisphere get no pollen data
- Note: `proxy/api/pollen.js` exists from TouchyWeather fork but is disabled
- Available: Google Pollen API (no auth required for public endpoints; requires reverse proxy for CORS)

**Implementation:**
- Re-enable the Google Pollen proxy (or refactor PKJS to call it directly)
- Extend `isEurope()` check to fall back to Google Pollen for non-CAMS regions
- Maintain backwards-compatible pollen level (-1 = unknown, 0–5 = UPI scale)

---

### 8. ⚡ Severe Weather Alerts
**Status:** Not available in Open-Meteo; requires external source  
**Impact:** High (public safety)  
**Details:**
- Currently: No alert system; storm is inferred from WMO codes (reactive, not predictive)
- Gap: Tornado watches, heat advisories, air quality alerts, hurricane warnings, winter storm warnings
- Available sources:
  - **US:** NWS alerts (free, public API, JSON via feed.weather.gov)
  - **Europe:** MeteoAlarm (free, public API)
  - **Global:** Various national met offices (non-standard APIs)
- Challenge: Pebble watch memory is tight; storing full alert text is impractical. A simple alert-present flag + category is more realistic.

**Implementation:**
- Add `alert_active` (bool) and `alert_category` (enum: WIND, HEAT, COLD, FLOOD, TORNADO, etc.) to `WeatherData`
- Create a simple alert fetcher in PKJS (NWS for US, MeteoAlarm for EU, fallback to none)
- Display alert badge on Main card or create a dedicated Alert card
- Keep it minimal — just "⚠️ HEAT ADVISORY" or similar

---

### 9. 🌧️ Daily Precipitation Accumulation
**Status:** Available in Open-Meteo, not fetched  
**Impact:** Medium (useful for rain/snow planning)  
**Details:**
- Currently: Week Ahead shows precipitation *probability* only
- Available: `daily.precipitation_sum` (total mm for the day)
- Use case: "60% chance" is vague; "60% chance, 8mm expected" means a shower; "60% chance, 25mm" means garden-soaking rain
- Complement: Show both probability and accumulation for each day

**Implementation:**
- Add `days_precip_amount[4]` to `WeatherData`
- Fetch `precipitation_sum` in daily request
- Display on Week Ahead card (e.g., "MON 70°/55° ☁ 60% · 5mm")

---

## Large Effort or Blocked

### 10. 🛰️ Radar / Satellite Imagery
**Status:** Available, but memory-constrained on watch  
**Impact:** Medium (live rain movement is useful)  
**Details:**
- Currently: Removed from TouchyWeather fork ("Radar card removed")
- Gap: No visual rain forecast; just icons and text
- Blocker: Pebble's framebuffer is ~96KB; a single radar tile is ≤50KB, but animated sequences or multiple tiles blow the budget
- Note: The proxy has a `proxy/api/radar.js` stub but it's unused

**Implementation:** (Low priority)
- Static radar tile as an optional card (one-time fetch, not animated)
- Tile URL from a free source (Rainviewer, NOAA, etc.)
- Cache locally to avoid repeated downloads
- Reality: Pebble's UI is so small that radar detail is marginal; text-based hourly bars are more readable

---

### 11. 📍 Multiple Location Support
**Status:** Partially implemented (location override in settings)  
**Impact:** Low–Medium (nice-to-have)  
**Details:**
- Currently: Single location only; user can override via lat/lon in settings
- Gap: No favorite locations; can't quickly switch between home and office
- Blocker: Watch memory and settings UI complexity; storing 5 locations + names eats into Pebble's budget

**Implementation:** (Low priority)
- Extend Clay config to accept multiple (name, lat, lon) tuples
- Add location selector card or button press to cycle
- Store in localStorage; sync on settings save
- Reality: TouchyWeather didn't have this; adds UI complexity for minimal gain on a small screen

---

## Summary Table

| Feature | Effort | Data Cost | Priority | Notes |
|---------|--------|-----------|----------|-------|
| Wind gusts | ⭐ | None | 🔴 High | Already fetched, just ignored |
| Hourly feels-like | ⭐ | None | 🟡 Medium | Partial (current only) |
| Precip amount (mm) | ⭐ | None | 🔴 High | Completes the picture |
| Visibility | ⭐ | None | 🟢 Low | Niche use case |
| Hourly/weekly wind | ⭐ | None | 🟡 Medium | Adds 12–24h planning detail |
| Extended hourly (12–24h) | ⭐⭐ | None | 🟡 Medium | UI challenge, not data |
| Pollen (non-EU) | ⭐⭐ | 1 extra call (cached) | 🟡 Medium | Code stub exists; proxy needed |
| Severe alerts | ⭐⭐⭐ | Multiple calls | 🔴 High | Public safety; needs NWS/MeteoAlarm |
| Daily precip amount | ⭐ | None | 🟡 Medium | Already available |
| Radar tiles | ⭐⭐ | Tile fetch | 🟢 Low | Memory-tight; marginal value |
| Multi-location | ⭐⭐⭐ | None | 🟢 Low | UI/memory overhead |

---

## Recommended Priority

**Quick wins (1–2 days):**
1. Wind gusts on Main card
2. Precipitation amount bars on Precip card
3. Hourly feels-like on Hours card

**Medium effort (2–3 days):**
4. Hourly/weekly wind display
5. Daily precip accumulation on Week card
6. Visibility display (if needed)

**Deferred (architecture needed):**
7. Severe alerts (requires NWS/MeteoAlarm integration)
8. Pollen for non-EU (requires proxy or direct Google call)
9. Extended hourly (UI layout rework)

---

## Implementation Notes

- Most changes are **additive** — existing cards don't need to shrink, just gain more detail
- Open-Meteo API **does not need changes** for items 1–6; the data is already in the response
- PKJS `src/pkjs/index.js` already parses and processes most of this; just needs storage in `WeatherData` struct
- Watch C code in `src/c/` needs rendering updates, but layout is already flexible per card
