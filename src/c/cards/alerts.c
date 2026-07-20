#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../weather_data.h"
#include "../anim.h"

// Returns the display label for an active alert category.
static const char *prv_alert_label(AlertCategory cat) {
  switch (cat) {
    case ALERT_CAT_TORNADO: return "TORNADO WARNING";
    case ALERT_CAT_WIND:    return "WIND ADVISORY";
    case ALERT_CAT_HEAT:    return "HEAT ADVISORY";
    case ALERT_CAT_COLD:    return "COLD ADVISORY";
    case ALERT_CAT_FLOOD:   return "FLOOD WARNING";
    case ALERT_CAT_WINTER:  return "WINTER STORM";
    case ALERT_CAT_OTHER:   return "WEATHER ALERT";
    default:                return "WEATHER ALERT";
  }
}

// Returns the accent color for an active alert category.
static GColor prv_alert_color(AlertCategory cat) {
  switch (cat) {
    case ALERT_CAT_TORNADO: return GColorRed;
    case ALERT_CAT_WIND:    return GColorOrange;
    case ALERT_CAT_HEAT:    return GColorOrange;   // theme_accent_orange() would also work
    case ALERT_CAT_COLD:    return GColorCyan;
    case ALERT_CAT_FLOOD:   return GColorBlue;
    case ALERT_CAT_WINTER:  return GColorLightGray;
    case ALERT_CAT_OTHER:   return GColorOrange;
    default:                return GColorOrange;
  }
}

void card_alerts_draw(GContext *ctx, GRect bounds) {
  WeatherData *d = weather_data_get();
  int W = bounds.size.w;
  int H = bounds.size.h;
  // Slide-transition origin offset: translate every coordinate by the
  // bounds origin so the entire card moves as one rigid unit.
  int ox = bounds.origin.x;
  int oy = bounds.origin.y;

  // Header with warning triangle icon.
  int header_y = UI_HEADER_Y;
  ui_draw_card_header_with_icon(ctx, bounds, "ALERTS",
                                theme_fg(),
                                header_y, 18, icon_draw_warning_triangle);

  // Vertical center of the content area below the header.
  int content_top = oy + header_y + UI_HEADER_HEIGHT + 8;
  int content_h   = H - (header_y + UI_HEADER_HEIGHT + 8) - 24; // 24 for banner
  int mid_y = content_top + content_h / 2;

  if (d->alert_category == ALERT_CAT_UNKNOWN) {
    // Region not covered by any alert source — show "NO DATA".
    graphics_context_set_text_color(ctx, theme_secondary());
    graphics_draw_text(ctx, "NO DATA",
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(ox, mid_y - 12, W, 24),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    graphics_context_set_text_color(ctx, theme_muted());
    graphics_draw_text(ctx, "Coverage unavailable",
        fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(ox + UI_MARGIN_X, mid_y + 14, W - UI_MARGIN_X * 2, 20),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else if (!d->alert_active) {
    // No active alert — show ALL CLEAR.
    int icon_size = PBL_IF_ROUND_ELSE(36, 32);
    GPoint icon_center = GPoint(ox + W / 2,
                                mid_y - icon_size / 2 - 4);
    icon_draw_sun(ctx, icon_center, icon_size, GColorIslamicGreen);
    graphics_context_set_text_color(ctx, GColorIslamicGreen);
    graphics_draw_text(ctx, "ALL CLEAR",
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(ox, mid_y + icon_size / 2 - 4, W, 24),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else {
    // Active alert — show warning icon + category label in severity color.
    GColor alert_color = prv_alert_color(d->alert_category);
    int icon_size = PBL_IF_ROUND_ELSE(40, 36);
    GPoint icon_center = GPoint(ox + W / 2,
                                mid_y - icon_size / 2 - 6);
    icon_draw_warning_triangle(ctx, icon_center, icon_size, alert_color);
    graphics_context_set_text_color(ctx, alert_color);
    graphics_draw_text(ctx, prv_alert_label(d->alert_category),
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(ox + UI_MARGIN_X, mid_y + icon_size / 2 - 4,
              W - UI_MARGIN_X * 2, 24),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  (void)H;
  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      d->fetch_error,
                      d->refresh_in_progress,
                      d->update_available,
                      anim_get_frame());
}
