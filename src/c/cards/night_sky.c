#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../anim.h"
#include "../weather_data.h"
#include <stdio.h>

// Phase 10C: Night Sky card with locked palette.
//
// Why locked colors: prior to this fix, the moon was drawn with
// theme_fg() as the body and theme_bg() as the shadow. In light mode
// that produced a black moon with a white shadow — visually inverted
// and confusing (a "waxing" crescent appeared waning). The phase
// shadow direction is mathematical and shouldn't flip with theme.
//
// Solution: always draw a small dark "sky" disc behind the moon, then
// the moon body in cream and shadow in the same dark sky color. The
// moon now reads correctly in either theme.

void card_night_sky_draw(GContext *ctx, GRect bounds) {
  WeatherData *d = weather_data_get();
  int W = bounds.size.w;
  int oy = bounds.origin.y;

  ui_draw_card_header_with_icon(ctx, bounds, "NIGHT SKY",
                                theme_fg(),
                                UI_HEADER_Y, 18, icon_draw_moon_small);

  // Locked moon palette — never derived from theme.
  const GColor moon_body   = GColorIcterine;     // cream-yellow
  const GColor moon_shadow = GColorOxfordBlue;   // deep navy

  int moon_size = PBL_IF_ROUND_ELSE(58, 56);
  // Tightened top gap (was 20/16) to give the auto-banner room below
  // the illumination line without overlapping it.
  int moon_y = oy + UI_HEADER_Y + UI_HEADER_HEIGHT + PBL_IF_ROUND_ELSE(10, 12) + moon_size/2;
  GPoint moon_c = GPoint(bounds.origin.x + W/2, moon_y);

  // Sky disc: ~7px larger than the moon so a thin navy ring frames
  // the moon regardless of theme. This is also what the moon-phase
  // shadow color matches, so the lit fraction reads cleanly.
  int sky_r = moon_size/2 + 7;
  graphics_context_set_fill_color(ctx, moon_shadow);
  graphics_fill_circle(ctx, moon_c, sky_r);

  // Moon glyph with the sky color as the shadow color.
  icon_draw_moon_phase(ctx, moon_c, moon_size, d->moon_phase,
                       moon_body, moon_shadow);

  // Mask shadow overflow: for non-full phases the offset shadow disc
  // extends past the sky disc edge horizontally (up to ~r past it for
  // the quarters). Without masking it shows up as a second navy disc
  // beside the moon. Two bg-color rectangles flanking the sky disc
  // mask any horizontal overflow without touching the header above
  // or labels below.
  int half = moon_size / 2;
  graphics_context_set_fill_color(ctx, theme_bg());
  graphics_fill_rect(ctx,
      GRect(bounds.origin.x, moon_c.y - half,
            (moon_c.x - sky_r) - bounds.origin.x, moon_size),
      0, GCornerNone);
  graphics_fill_rect(ctx,
      GRect(moon_c.x + sky_r, moon_c.y - half,
            (bounds.origin.x + W) - (moon_c.x + sky_r), moon_size),
      0, GCornerNone);

  // Phase name word 1 (large, fg).
  int name1_y = moon_y + moon_size/2 + PBL_IF_ROUND_ELSE(10, 8);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, d->moon_name1,
      fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
      GRect(bounds.origin.x, name1_y, W, 32),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Phase name word 2 (smaller, secondary — readable on white).
  int name2_y = name1_y + 28;
  graphics_context_set_text_color(ctx, theme_secondary());
  graphics_draw_text(ctx, d->moon_name2,
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(bounds.origin.x, name2_y, W, 22),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Illumination percentage.
  char illum_buf[12];
  snprintf(illum_buf, sizeof(illum_buf), "%d%% LIT", (int)d->moon_illum);
  int illum_y = name2_y + 22;
  graphics_context_set_text_color(ctx, theme_secondary());
  graphics_draw_text(ctx, illum_buf,
      fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
      GRect(bounds.origin.x, illum_y, W, 18),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      d->update_failed,
                      d->refresh_in_progress,
                      anim_get_frame());
}
