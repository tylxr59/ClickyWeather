# Changelog

## 1.4.0

- Fixed location, precipitation-unit, air-quality, pollen, and weather-alert failure handling so the watch no longer presents fallback or stale values as current conditions.
- Improved refresh reliability with request sequencing, explicit timeouts, parallel network requests, and complete settings restoration.
- Added automated weather-data tests, reproducible release tooling, and clearer privacy documentation for every network service.

## 1.3.1

- Added configurable app update checks with Never, every weather refresh, timed intervals, and a Check Now button.
- Fixed stale update notices after installing a different app version.
- Improved recovery from temporary GitHub failures so one failed request does not suppress checks for the full selected interval.

## 1.3.0

- Added detailed views for hourly temperature, rainfall, the five-day forecast, UV, and air quality; hold SELECT on a supported card to open one.
- Added an AppGlance launcher summary and reduced unnecessary weather requests.
- Improved background updates, timezone handling, units, AQI and pollen labels, precipitation charts, and unknown UV display.
- Added one-time on-watch release notes and privacy-first project rules that prohibit analytics and tracking.

## 1.2.0

- Added configurable background weather refresh and 12/24-hour time formatting.
- Added an on-watch notice when a newer GitHub release is available.
- Automated tagged PBW builds and GitHub Releases.

## 1.1.0

- Added the initial ClickyWeather release workflow and fork attribution.
