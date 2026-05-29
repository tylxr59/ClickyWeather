# AQI Gauge Expression — Plan

## Problem

The Air Quality card's half-arc gauge in [`card_air_quality_draw()`](src/c/cards/air_quality.c:23) currently uses a straight linear map:

```c
int sweep_deg = (aqi_capped * 180) / 300;
```

Two issues with this expression:

1. **AQI 60 only fills 20% of the arc.** Modest "Moderate" values feel visually underwhelming relative to how the number reads. Real-world AQI 60 reports look like nearly-empty dials, even though 60 is no longer in the "Good" range.
2. **AQI 301–500 is not representable.** The cap is 300, so anything from 300 to 500 (the full EPA "Hazardous" range) shows as a fully-pegged dial with no differentiation.

## Decision: Option C — Healthy/Unhealthy Halves

Anchor AQI 100 (the Good/Moderate → Unhealthy-for-Sensitive-Groups inflection point) at the **top of the arc** (90° / 50% fill). This gives the gauge a clear semantic story:

- **Left half** of the arc = AQI 0–100 = everyday-acceptable
- **Right half** of the arc = AQI 100–500 = take action

The right half is segmented piecewise by EPA breakpoints so each "unhealthy" category gets visible territory rather than being squeezed.

## Mapping

| AQI range | Sweep range | EPA category |
|-----------|-------------|--------------|
| 0–100     | 0°–90°      | Good + Moderate (linear) |
| 100–150   | 90°–113°    | Unhealthy for Sensitive Groups |
| 150–200   | 113°–135°   | Unhealthy |
| 200–300   | 135°–158°   | Very Unhealthy |
| 300–500   | 158°–180°   | Hazardous |

## Before / After at common values

| AQI | Current fill | New fill | Δ | Notes |
|-----|--------------|----------|------|-------|
| 25  | 8%   | 13%  | +5 pts  | Good — still clearly low |
| 33  | 11%  | 16%  | +5 pts  | Reference screenshot value |
| 50  | 17%  | 25%  | +8 pts  | End of "Good" band |
| 60  | 20%  | 30%  | **+10 pts** | The motivating complaint — now feels meaningful |
| 75  | 25%  | 38%  | +13 pts | Mid-Moderate |
| 100 | 33%  | **50%** | +17 pts | Anchored at top of arc |
| 125 | 42%  | 56%  | +14 pts | Mid-USG |
| 150 | **50%** | 63%  | +13 pts | Was at top, now past it |
| 200 | 67%  | 75%  | +8 pts  | |
| 250 | 83%  | 81%  | -2 pts  | Roughly equal |
| 300 | **100%** | 88%  | -12 pts | No longer pegged |
| 400 | (pegged) | 94%  | — | Newly representable |
| 500 | (pegged) | **100%** | — | Newly representable |

**Tradeoff:** AQI 300 no longer fills the dial completely. In exchange, AQI 100 sits at the halfway mark and AQI 301–500 is now visually distinguishable. Net win — AQI 300+ is rare and previously ambiguous (every value 300+ looked identical).

## Implementation

**File:** [`src/c/cards/air_quality.c`](src/c/cards/air_quality.c)

### 1. Add helper above `card_air_quality_draw()`

```c
// Phase 10E.x: AQI → arc sweep mapping.
//
// "Healthy/unhealthy halves" expression:
//   AQI 0–100   → 0–90°   (left half of the arc)
//   AQI 100–500 → 90–180° (right half, piecewise by EPA band)
//
// Anchoring AQI 100 (the Good/Moderate → USG inflection) at the top of
// the arc makes the gauge legible at a glance: left half = everyday-
// acceptable, right half = take action. Also extends honest support to
// AQI 301–500 instead of clipping at 300 like the previous linear
// formula did.
static int aqi_to_sweep_deg(int aqi) {
  if (aqi <= 0) return 0;
  if (aqi <= 100) return (aqi * 90) / 100;
  // Upper-half breakpoints: AQI 100→90°, 150→113°, 200→135°, 300→158°, 500→180°
  static const int bp[]  = {100, 150, 200, 300, 500};
  static const int deg[] = { 90, 113, 135, 158, 180};
  for (int i = 1; i < 5; i++) {
    if (aqi <= bp[i]) {
      return deg[i-1] + ((aqi - bp[i-1]) * (deg[i] - deg[i-1])) / (bp[i] - bp[i-1]);
    }
  }
  return 180;
}
```

### 2. Replace the cap-and-compute block (lines ~47–50)

Before:
```c
int aqi_capped = d->aqi;
if (aqi_capped < 0) aqi_capped = 0;
if (aqi_capped > 300) aqi_capped = 300;
int sweep_deg = (aqi_capped * 180) / 300;
```

After:
```c
int aqi_capped = d->aqi;
if (aqi_capped < 0) aqi_capped = 0;
if (aqi_capped > 500) aqi_capped = 500;
int sweep_deg = aqi_to_sweep_deg(aqi_capped);
```

## Risks & Notes

- Pure integer math, no float, no new dependencies. Trivial CPU cost (one branch + one short loop, max 4 iterations).
- Only the arc sweep changes. Color bands ([`aqi_category_color()`](src/c/cards/air_quality.c:14)), the hero number, the GOOD/MODERATE/etc. label, and the pollen badge are untouched. EPA categories still drive color truthfully.
- No data model, comm, or API changes.
- Behavior on AQI < 0 (unknown / not yet fetched) is unchanged.
- Visual check on Emery and Gabbro at AQI 33, 60, 100, 150 worth doing post-implementation to confirm sweep looks right alongside color transitions.