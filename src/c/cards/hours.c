#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../anim.h"
#include "../weather_data.h"
#include <stdio.h>

// Phase 10A: Next 6 Hours card. 6 stacked rows of
// (time, icon, temp, optional precip%). Precip droplet + percent only
// renders when probability >= POP_THRESHOLD — otherwise the column
// collapses so dry rows aren't visually noisy.
//
// Layout strategy: uniform-column cluster centering. We measure the
// widest time and temp across all 6 rows, then center the
// `time | icon | temp | precip` cluster horizontally. Rows align
// because each column has a uniform width.

#define POP_THRESHOLD 10

static GColor cond_accent(WeatherCondition cond) {
  switch (cond) {
    case COND_SUNNY:
    case COND_PARTLY_CLOUDY:
      return theme_accent_orange();
    case COND_RAIN:
    case COND_SNOW:
    case COND_STORM:
      return theme_accent_blue();
    default:
      return theme_fg();
  }
}

void card_hours_draw(GContext *ctx, GRect bounds) {
  WeatherData *d = weather_data_get();
  int W = bounds.size.w;
  int oy = bounds.origin.y;

  ui_draw_card_header_with_icon(ctx, bounds, "6 HOURS",
                                theme_fg(),
                                UI_HEADER_Y, 18, icon_draw_clock);

  // Tighter row font so 6 rows fit without crowding the page indicator.
  GFont row_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  GFont pop_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  int icon_size = 16;
  int gap = 8;
  int row_h = PBL_IF_ROUND_ELSE(20, 19);
  int top_y = oy + UI_HEADER_Y + UI_HEADER_HEIGHT + PBL_IF_ROUND_ELSE(10, 6);

  // Measure the widest time, temp, and pop across all rows for uniform
  // column widths. Pop column collapses if no row meets the threshold.
  GSize time_max = GSize(0, 0);
  GSize temp_max = GSize(0, 0);
  GSize pop_max  = GSize(0, 0);
  bool any_pop = false;
  char buf[12];
  for (int i = 0; i < 6; ++i) {
    GSize ts = graphics_text_layout_get_content_size(d->hours_label[i],
        row_font, GRect(0,0,W,30), GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft);
    if (ts.w > time_max.w) time_max = ts;
    snprintf(buf, sizeof(buf), "%d°", d->hours_temp[i]);
    GSize ps = graphics_text_layout_get_content_size(buf, row_font,
        GRect(0,0,W,30), GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft);
    if (ps.w > temp_max.w) temp_max = ps;
    if (d->hours_pop[i] >= POP_THRESHOLD) {
      any_pop = true;
      snprintf(buf, sizeof(buf), "%d%%", (int)d->hours_pop[i]);
      GSize qs = graphics_text_layout_get_content_size(buf, pop_font,
          GRect(0,0,W,30), GTextOverflowModeTrailingEllipsis,
          GTextAlignmentLeft);
      if (qs.w > pop_max.w) pop_max = qs;
    }
  }

  int pop_icon = 10;
  int pop_col_w = any_pop ? (pop_icon + 4 + pop_max.w) : 0;
  int cluster_w = time_max.w + gap + icon_size + gap + temp_max.w +
                  (any_pop ? (gap + pop_col_w) : 0);
  int cluster_x = bounds.origin.x + (W - cluster_w) / 2;
  int floor_x = bounds.origin.x + UI_MARGIN_X;
  if (cluster_x < floor_x) cluster_x = floor_x;

  for (int i = 0; i < 6; ++i) {
    int row_y = top_y + i * row_h;
    int icon_cx = cluster_x + time_max.w + gap + icon_size / 2;
    int icon_cy = row_y + 10;

    graphics_context_set_text_color(ctx, theme_fg());
    graphics_draw_text(ctx, d->hours_label[i], row_font,
        GRect(cluster_x, row_y - 2, time_max.w + 4, 22),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    icon_draw_condition(ctx, GPoint(icon_cx, icon_cy),
                        icon_size, d->hours_cond[i]);

    snprintf(buf, sizeof(buf), "%d°", d->hours_temp[i]);
    int temp_x = cluster_x + time_max.w + gap + icon_size + gap;
    graphics_context_set_text_color(ctx, cond_accent(d->hours_cond[i]));
    graphics_draw_text(ctx, buf, row_font,
        GRect(temp_x, row_y - 2, temp_max.w + 4, 22),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    if (any_pop && d->hours_pop[i] >= POP_THRESHOLD) {
      int pop_x = temp_x + temp_max.w + gap;
      icon_draw_droplet(ctx,
          GPoint(pop_x + pop_icon/2, icon_cy),
          pop_icon, theme_accent_blue());
      snprintf(buf, sizeof(buf), "%d%%", (int)d->hours_pop[i]);
      graphics_context_set_text_color(ctx, theme_accent_blue());
      graphics_draw_text(ctx, buf, pop_font,
          GRect(pop_x + pop_icon + 4, row_y, pop_max.w + 4, 18),
          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }
  }

  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      d->update_failed,
                      anim_get_frame());
}
