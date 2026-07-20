#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../weather_data.h"
#include "../anim.h"
#include <stdio.h>

void card_uv_draw(GContext *ctx, GRect bounds) {
  WeatherData *d = weather_data_get();
  int W = bounds.size.w;
  int H = bounds.size.h;
  // Slide-transition origin offset: translate every coordinate by the
  // bounds origin so the entire card moves as one rigid unit.
  int ox = bounds.origin.x;
  int oy = bounds.origin.y;

  // Header: small sun + "UV INDEX".
  int header_y = UI_HEADER_Y;
  // Header in fg for legibility on either theme. The orange accent
  // lives on the gauge body where it carries the actual UV signal.
  ui_draw_card_header_with_icon(ctx, bounds, "UV INDEX",
                                theme_fg(),
                                header_y, 18, icon_draw_sun);

  // Half-arc gauge (180° from -90 to +90).
  int radius = PBL_IF_ROUND_ELSE(72, 64);
  int thickness = 10;
  GPoint c = { ox + W/2, oy + header_y + UI_HEADER_HEIGHT + 8 + radius };
  GRect arc_box = GRect(c.x - radius, c.y - radius, radius*2, radius*2);
  // Background track.
  graphics_context_set_fill_color(ctx, theme_muted());
  graphics_fill_radial(ctx, arc_box, GOvalScaleModeFitCircle, thickness,
                       DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));
  bool uv_unknown = d->uv < 0;
  int uv = uv_unknown ? 0 : d->uv;
  // Foreground proportion: uv 0..11 over 180°; unknown leaves it empty.
  int uv_capped = uv > 11 ? 11 : uv;
  int sweep_deg = (uv_capped * 180) / 11;
  graphics_context_set_fill_color(ctx, theme_accent_orange());
  graphics_fill_radial(ctx, arc_box, GOvalScaleModeFitCircle, thickness,
                       DEG_TO_TRIGANGLE(-90),
                       DEG_TO_TRIGANGLE(-90 + sweep_deg));

  // Big number inside.
  char buf[8];
  snprintf(buf, sizeof(buf), uv_unknown ? "?" : "%d", uv);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, buf,
      fonts_get_system_font(uv_unknown ? FONT_KEY_BITHAM_42_BOLD
                                       : FONT_KEY_LECO_42_NUMBERS),
      GRect(c.x - radius, c.y - 32, radius*2, 50),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Label below.
  graphics_draw_text(ctx, uv_unknown ? "UNKNOWN" : uv_label(uv),
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(ox, c.y + 18, W, 24),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // "PEAK n" subtitle in the secondary color, just below the qualitative
  // label. Suppressed when uv_max is 0/unknown so we don't show "PEAK 0"
  // before any daytime refresh has populated the daily peak.
  if (d->uv_max > 0) {
    char peak_buf[16];
    snprintf(peak_buf, sizeof(peak_buf), "PEAK %d", d->uv_max);
    graphics_context_set_text_color(ctx, theme_secondary());
    graphics_draw_text(ctx, peak_buf,
        fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(ox, c.y + 38, W, 18),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  (void)H;
  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      d->update_failed,
                      d->refresh_in_progress,
                      d->update_available,
                      anim_get_frame());
}
