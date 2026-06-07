#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../anim.h"
#include "../weather_data.h"
#include <stdio.h>

// Phase 10A: Next 6 Hours card. 6 stacked rows of
// (time, icon, temp, context). The context column is mutually exclusive
// per row: when the hour is precipitating it shows a droplet + amount
// (inches imperial / mm metric); otherwise it shows a compass arrow +
// wind speed. Dry + calm rows collapse the column entirely. Showing only
// one of the two guarantees the cluster fits even on gabbro's narrow
// top/bottom rows.
//
// Layout strategy: uniform-column cluster centering. We measure the
// widest time, temp, and context across all 6 rows, then center the
// `time | icon | temp | context` cluster horizontally. Rows align
// because each column has a uniform width.

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

// Per-row context column kinds.
typedef enum { CTX_NONE = 0, CTX_PRECIP, CTX_WIND } HourCtx;

// Decides which context column a row shows and formats its text into buf.
// Returns the kind. Precip wins when there is a measurable amount.
static HourCtx hour_ctx(const WeatherData *d, int i, char *buf, size_t n) {
  if (d->hours_precip_x10[i] > 0) {
    if (d->units == UNITS_METRIC) {
      int mm = (d->hours_precip_x10[i] + 9) / 10; // ceil, always >= 1
      snprintf(buf, n, "%dmm", mm);
    } else {
      int x = d->hours_precip_x10[i];
      snprintf(buf, n, "%d.%d\"", x / 10, x % 10);
    }
    return CTX_PRECIP;
  }
  if (d->hours_wind[i] > 0) {
    snprintf(buf, n, "%d", d->hours_wind[i]);
    return CTX_WIND;
  }
  buf[0] = '\0';
  return CTX_NONE;
}

void card_hours_draw(GContext *ctx, GRect bounds) {
  WeatherData *d = weather_data_get();
  int W = bounds.size.w;

  ui_draw_card_header_with_icon(ctx, bounds, "6 HOURS",
                                theme_fg(),
                                UI_HEADER_Y, 18, icon_draw_clock);

  // Tighter row font so 6 rows fit without crowding the page indicator.
  GFont row_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  GFont pop_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  int icon_size = 16;
  int gap = 5;
  int row_h = PBL_IF_ROUND_ELSE(20, 19);
  int top_y = UI_HEADER_Y + UI_HEADER_HEIGHT + PBL_IF_ROUND_ELSE(10, 6);

  // Context column glyph widths.
  int drop_icon  = 10;
  int arrow_icon = 12;
  int ctx_text_gap = 4;

  // Measure the widest time, temp, and context across all rows for
  // uniform column widths. The context column collapses if every row is
  // dry + calm.
  GSize time_max = GSize(0, 0);
  GSize temp_max = GSize(0, 0);
  int   ctx_max_w = 0;
  bool  any_ctx = false;
  char buf[12];
  char ctxbuf[12];
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

    HourCtx kind = hour_ctx(d, i, ctxbuf, sizeof(ctxbuf));
    if (kind != CTX_NONE) {
      any_ctx = true;
      GSize cs = graphics_text_layout_get_content_size(ctxbuf, pop_font,
          GRect(0,0,W,30), GTextOverflowModeTrailingEllipsis,
          GTextAlignmentLeft);
      int glyph = (kind == CTX_PRECIP) ? drop_icon : arrow_icon;
      int w = glyph + ctx_text_gap + cs.w;
      if (w > ctx_max_w) ctx_max_w = w;
    }
  }

  int cluster_w = time_max.w + gap + icon_size + gap + temp_max.w +
                  (any_ctx ? (gap + ctx_max_w) : 0);
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

    if (any_ctx) {
      HourCtx kind = hour_ctx(d, i, ctxbuf, sizeof(ctxbuf));
      int ctx_x = temp_x + temp_max.w + gap;
      if (kind == CTX_PRECIP) {
        icon_draw_droplet(ctx,
            GPoint(ctx_x + drop_icon/2, icon_cy),
            drop_icon, theme_accent_blue());
        graphics_context_set_text_color(ctx, theme_accent_blue());
        graphics_draw_text(ctx, ctxbuf, pop_font,
            GRect(ctx_x + drop_icon + ctx_text_gap, row_y, ctx_max_w, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      } else if (kind == CTX_WIND) {
        icon_draw_compass_arrow(ctx,
            GPoint(ctx_x + arrow_icon/2, icon_cy),
            arrow_icon, theme_fg(), d->hours_wind_dir[i]);
        graphics_context_set_text_color(ctx, theme_fg());
        graphics_draw_text(ctx, ctxbuf, pop_font,
            GRect(ctx_x + arrow_icon + ctx_text_gap, row_y, ctx_max_w, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      }
    }
  }

  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      anim_get_frame());
}
