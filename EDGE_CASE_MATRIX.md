# Touch and Go Edge-Case Matrix

This matrix is used to verify deterministic classifier behavior in src/c/cards/advice.c.

Rules:
- Tier result must match the conflict-priority rubric in code.
- Headline must reflect the winning trigger metric.
- Phrase must come from the expected pool family.
- Any disallowed outcome is considered a failure.

## Deterministic Execution Mode

This matrix can be replayed deterministically using compile-time case selection in src/c/cards/advice.c.

Procedure:
1. In src/c/cards/advice.c, set TW_ADVICE_MATRIX_CASE to the desired case number (1..15).
2. Build and install on emulator.
3. Navigate to Touch and Go card.
4. Record tier/headline/phrase-family outcome for emery and gabbro.
5. Reset TW_ADVICE_MATRIX_CASE back to 0 when matrix run is complete.

Case id mapping:
- 1..15 maps to M01..M15 in order.

## Cases

| ID | Scenario | Representative Inputs | Expected Tier | Expected Headline Pattern | Expected Phrase Family | Disallowed Outcomes |
|---|---|---|---|---|---|---|
| M01 | Stale packet | valid=true, last_updated older than 3h | CHECK | DATA MAY BE STALE | PHRASES_DATA_STALE | Any confident weather directive |
| M02 | Invalid packet | valid=false | CHECK | DATA MAY BE STALE | PHRASES_DATA_STALE | Any non-stale tier |
| M03 | Active storm | condition=COND_STORM | STORM | STORM OVERHEAD | PHRASES_STORM | RAINY, WINDY, NICE |
| M04 | Rain soon with wind | rain_alert_min <= 30, wind >= threshold | RAIN SOON | RAIN IN X MINS | PHRASES_RAIN_SOON | WINDY winning over rain soon |
| M05 | Rain now near freezing | condition=COND_RAIN, feels_like <= freezingish | RAINY | COLD RAIN / X° | PHRASES_RAIN_COLD | Generic rain pool, NICE |
| M06 | Snow now | condition=COND_SNOW | SNOW | SNOWING / X° | PHRASES_SNOW | RAINY or NICE |
| M07 | Hot stress | feels_like >= hot threshold | HOT | FEELS LIKE X° | PHRASES_HOT | NICE |
| M08 | Cold stress | feels_like <= cold threshold | COLD | FEELS LIKE X° | PHRASES_COLD | NICE |
| M09 | Late-night cool fallback | night, hour in [22..5], feels_like <= cool fallback threshold | COLD | FEELS LIKE X° | PHRASES_COLD | NICE at late night cool |
| M10 | High wind only | wind >= wind threshold, no higher-priority hazards | WINDY | X MPH GUSTS or X KMH GUSTS | PHRASES_WIND | NICE |
| M11 | Daytime severe UV | daytime=true, uv >= 8, no higher-priority hazards | HIGH UV | UV INDEX: X | PHRASES_HIGH_UV | UV tier at night |
| M12 | Bad air and muggy | aqi >= 100 and humidity/temp muggy thresholds met | BAD AIR | AQI: X (POOR) | PHRASES_BAD_AIR | MUGGY winning over BAD AIR |
| M13 | Pleasant daytime mild | no trigger above pleasant | NICE | X° & CLEAR | PHRASES_PLEASANT_DAY | Any hazard tier |
| M14 | Pleasant nighttime mild | no hazard, daytime=false | NICE | NIGHT / X° | PHRASES_PLEASANT_NIGHT | Daytime pleasant pool |
| M15 | Pleasant daytime cool | daytime=true, no hazards, feels_like in cool band (imperial 46..55, metric 8..13) | NICE | X° & CLEAR | PHRASES_PLEASANT_COOL | PHRASES_PLEASANT_DAY |

## Execution Log Template

Use this table during emulator validation.

| ID | emery result | gabbro result | pass/fail | notes |
|---|---|---|---|---|
| M01 | CHECK + DATA MAY BE STALE, phrase from PHRASES_DATA_STALE | CHECK + DATA MAY BE STALE, phrase from PHRASES_DATA_STALE | pass | screenshots/matrix/M01_emery.png, screenshots/matrix/M01_gabbro.png |
| M02 | CHECK + DATA MAY BE STALE, phrase from PHRASES_DATA_STALE | CHECK + DATA MAY BE STALE, phrase from PHRASES_DATA_STALE | pass | screenshots/matrix/M02_emery.png, screenshots/matrix/M02_gabbro.png |
| M03 | STORM + STORM OVERHEAD, phrase from PHRASES_STORM | STORM + STORM OVERHEAD, phrase from PHRASES_STORM | pass | screenshots/matrix/M03_emery.png, screenshots/matrix/M03_gabbro.png |
| M04 | RAIN SOON + RAIN IN 10 MINS, phrase from PHRASES_RAIN_SOON | RAIN SOON + RAIN IN 10 MINS, phrase from PHRASES_RAIN_SOON | pass | screenshots/matrix/M04_emery.png, screenshots/matrix/M04_gabbro.png |
| M05 | RAINY + COLD RAIN / 33°, phrase from PHRASES_RAIN_COLD | RAINY + COLD RAIN / 33°, phrase from PHRASES_RAIN_COLD | pass | screenshots/matrix/M05_emery.png, screenshots/matrix/M05_gabbro.png |
| M06 | SNOW + SNOWING / 28°, phrase from PHRASES_SNOW | SNOW + SNOWING / 28°, phrase from PHRASES_SNOW | pass | screenshots/matrix/M06_emery.png, screenshots/matrix/M06_gabbro.png |
| M07 | HOT + FEELS LIKE 95°, phrase from PHRASES_HOT | HOT + FEELS LIKE 95°, phrase from PHRASES_HOT | pass | screenshots/matrix/M07_emery.png, screenshots/matrix/M07_gabbro.png |
| M08 | COLD + FEELS LIKE 38°, phrase from PHRASES_COLD | COLD + FEELS LIKE 38°, phrase from PHRASES_COLD | pass | screenshots/matrix/M08_emery.png, screenshots/matrix/M08_gabbro.png |
| M09 | COLD + FEELS LIKE 53°, phrase from PHRASES_COLD | COLD + FEELS LIKE 53°, phrase from PHRASES_COLD | pass | screenshots/matrix/M09_emery.png, screenshots/matrix/M09_gabbro.png |
| M10 | WINDY + 23 MPH GUSTS, phrase from PHRASES_WIND | WINDY + 23 MPH GUSTS, phrase from PHRASES_WIND | pass | screenshots/matrix/M10_emery.png, screenshots/matrix/M10_gabbro.png |
| M11 | HIGH UV + UV INDEX: 10, phrase from PHRASES_HIGH_UV | HIGH UV + UV INDEX: 10, phrase from PHRASES_HIGH_UV | pass | screenshots/matrix/M11_emery.png, screenshots/matrix/M11_gabbro.png |
| M12 | BAD AIR + AQI: 140 (POOR), phrase from PHRASES_BAD_AIR | BAD AIR + AQI: 140 (POOR), phrase from PHRASES_BAD_AIR | pass | screenshots/matrix/M12_emery.png, screenshots/matrix/M12_gabbro.png |
| M13 | NICE + 72° & CLEAR, phrase from PHRASES_PLEASANT_DAY | NICE + 72° & CLEAR, phrase from PHRASES_PLEASANT_DAY | pass | screenshots/matrix/M13_emery.png, screenshots/matrix/M13_gabbro.png |
| M14 | NICE + NIGHT / 64°, phrase from PHRASES_PLEASANT_NIGHT | NICE + NIGHT / 64°, phrase from PHRASES_PLEASANT_NIGHT | pass | screenshots/matrix/M14_emery.png, screenshots/matrix/M14_gabbro.png |
| M15 | NICE + 56° & CLEAR, phrase from PHRASES_PLEASANT_COOL | NICE + 56° & CLEAR, phrase from PHRASES_PLEASANT_COOL | pass | screenshots/matrix/M15_emery.png, screenshots/matrix/M15_gabbro.png |
