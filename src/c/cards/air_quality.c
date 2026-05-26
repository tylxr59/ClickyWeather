#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../weather_data.h"
#include "../anim.h"
#include <stdio.h>

// Phase 10E: AQI category color helper.
//
// Maps the EPA AQI bands to readable Pebble palette colors. Used both
// for the gauge sweep and the hero number so the value's severity is
// communicated by color even at a glance.
static GColor aqi_category_color(int aqi) {
  if (aqi <= 50)   return GColorIslamicGreen;       // GOOD
  if (aqi <= 100)  return theme_accent_orange();    // MODERATE (yellow)
  if (aqi <= 150)  return GColorOrange;             // UNHEALTHY FOR SENSITIVE
  if (aqi <= 200)  return GColorRed;                // UNHEALTHY
  if (aqi <= 300)  return GColorPurple;             // VERY UNHEALTHY
  return GColorDarkCandyAppleRed;                   // HAZARDOUS
}

void card_air_quality_draw(GContext *ctx, GRect bounds) {
  WeatherData *d = weather_data_get();
  int W = bounds.size.w;
  int H = bounds.size.h;
  // Slide-transition origin offset: translate every coordinate by the
  // bounds origin so the entire card moves as one rigid unit.
  int ox = bounds.origin.x;
  int oy = bounds.origin.y;

  // Header in fg for legibility; the category color carries the signal.
  int header_y = UI_HEADER_Y;
  ui_draw_card_header_with_icon(ctx, bounds, "AIR QUALITY",
                                theme_fg(),
                                header_y, 18, icon_draw_pulse);

  // Half-arc gauge — same style as UV (consistent visual language).
  // AQI 0..300 maps to 0..180°. Values >300 max out the arc but the
  // numeric label still reads truthfully.
  int radius = PBL_IF_ROUND_ELSE(72, 64);
  int thickness = 10;
  GPoint c = { ox + W/2, oy + header_y + UI_HEADER_HEIGHT + 8 + radius };
  GRect arc_box = GRect(c.x - radius, c.y - radius, radius*2, radius*2);
  graphics_context_set_fill_color(ctx, theme_muted());
  graphics_fill_radial(ctx, arc_box, GOvalScaleModeFitCircle, thickness,
                       DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));
  int aqi_capped = d->aqi;
  if (aqi_capped < 0) aqi_capped = 0;
  if (aqi_capped > 300) aqi_capped = 300;
  int sweep_deg = (aqi_capped * 180) / 300;
  GColor cat = aqi_category_color(d->aqi);
  graphics_context_set_fill_color(ctx, cat);
  graphics_fill_radial(ctx, arc_box, GOvalScaleModeFitCircle, thickness,
                       DEG_TO_TRIGANGLE(-90),
                       DEG_TO_TRIGANGLE(-90 + sweep_deg));

  // Big AQI number inside, colored by category.
  char buf[8]; snprintf(buf, sizeof(buf), "%d", d->aqi);
  graphics_context_set_text_color(ctx, cat);
  graphics_draw_text(ctx, buf,
      fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
      GRect(c.x - radius, c.y - 32, radius*2, 50),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Label below — also colored by category.
  graphics_context_set_text_color(ctx, cat);
  graphics_draw_text(ctx, aqi_label(d->aqi),
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(ox, c.y + 18, W, 24),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Pollen badge (Google UPI 0..5). Skipped when uncovered (-1) so the
  // card looks unchanged for users outside Google's coverage region.
  if (d->pollen_level >= 0) {
    const char *plabel;
    GColor pcolor;
    int lvl = d->pollen_level;
    if (lvl == 0)      { plabel = "POLLEN: NONE";      pcolor = theme_secondary(); }
    else if (lvl == 1) { plabel = "POLLEN: VERY LOW";     pcolor = GColorIslamicGreen; }
    else if (lvl == 2) { plabel = "POLLEN: LOW";       pcolor = GColorIslamicGreen; }
    else if (lvl == 3) { plabel = "POLLEN: MODERATE";  pcolor = theme_accent_orange(); }
    else if (lvl == 4) { plabel = "POLLEN: HIGH";      pcolor = GColorOrange; }
    else               { plabel = "POLLEN: VERY HIGH";    pcolor = GColorRed; }
    graphics_context_set_text_color(ctx, pcolor);
    graphics_draw_text(ctx, plabel,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(ox, c.y + 40, W, 24),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  (void)H;
  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      d->update_failed,
                      anim_get_frame());
}
