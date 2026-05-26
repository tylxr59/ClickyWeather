#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../weather_data.h"
#include "../anim.h"
#include <stdio.h>

void card_main_draw(GContext *ctx, GRect bounds) {
  WeatherData *d = weather_data_get();
  int W = bounds.size.w;
  int H = bounds.size.h;
  int margin = UI_MARGIN_X;
  // Vertical slide transition: translate every coordinate by the bounds
  // origin so the entire card moves as one rigid unit during the slide.
  int ox = bounds.origin.x;
  int oy = bounds.origin.y;

  // Hero condition icon, centered, top.
  int icon_size = 48;
  int icon_y = PBL_IF_ROUND_ELSE(20, 14);
  icon_draw_condition_animated(ctx, GPoint(ox + W / 2, oy + icon_y + icon_size/2),
                               icon_size, d->condition, anim_get_frame());

  // Big temperature "72°" — left-aligned at the margin so it doesn't
  // collide with the Hi/Lo column on the right. Symmetric margins on
  // both sides of the card; the *visible* text just takes whatever
  // width LECO_42 needs.
  char temp_buf[8];
  snprintf(temp_buf, sizeof(temp_buf), "%d°", d->temp);
  GFont temp_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  int temp_y = PBL_IF_ROUND_ELSE(40, 40);
  int temp_h = 50;
  int hilo_w = 58;
  GRect temp_r = GRect(bounds.origin.x + margin, oy + temp_y,
                       W - 2 * margin - hilo_w, temp_h);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, temp_buf, temp_font, temp_r,
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);

  // Hi/Lo column on the right. Arrow immediately left of the number,
  // both left-aligned within the hilo_w column so they stay together.
  // Using GOTHIC_24_BOLD: cap-height ~18px, top bearing ~4px in a 28px box.
  // Arrow center is tuned to sit at mid cap-height (~13px below box top).
  int hilo_x = bounds.origin.x + W - margin - hilo_w + 4;
  int arrow_size = 12;
  GFont hilo_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  // Up arrow + high temp.
  icon_draw_arrow_up(ctx, GPoint(hilo_x + arrow_size/2, oy + temp_y + 15), arrow_size,
                     theme_accent_orange());
  char hi_buf[8]; snprintf(hi_buf, sizeof(hi_buf), "%d°", d->high);
  graphics_context_set_text_color(ctx, theme_accent_orange());
  graphics_draw_text(ctx, hi_buf, hilo_font,
                     GRect(hilo_x + arrow_size + 5, oy + temp_y, hilo_w - arrow_size - 5, 28),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  // Down arrow + low temp.
  icon_draw_arrow_down(ctx, GPoint(hilo_x + arrow_size/2, oy + temp_y + 39), arrow_size,
                       theme_accent_blue());
  char lo_buf[8]; snprintf(lo_buf, sizeof(lo_buf), "%d°", d->low);
  graphics_context_set_text_color(ctx, theme_accent_blue());
  graphics_draw_text(ctx, lo_buf, hilo_font,
                     GRect(hilo_x + arrow_size + 5, oy + temp_y + 22, hilo_w - arrow_size - 5, 28),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // "FEELS 75°" centered below big temp — bumped to 24_BOLD.
  char feels_buf[16];
  snprintf(feels_buf, sizeof(feels_buf), "FEELS %d°", d->feels_like);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, feels_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(ox, oy + temp_y + temp_h, W, 30),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Bottom split row: wind | humidity. Sits above the rain banner.
  // Pulled up vs original to make room for the second detail row below.
  // row_y = row2_y - 36. row2_y anchored from banner top: H - pad_bottom(22/40) - banner_h(22) - 22.
  int row_y = PBL_IF_ROUND_ELSE(H - 120, H - 102);
  // Vertical divider.
  graphics_context_set_stroke_color(ctx, theme_muted());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(ox + W/2, oy + row_y), GPoint(ox + W/2, oy + row_y + 32));

  // Wind (left column). Speed unit follows the user's selected system.
  icon_draw_wind(ctx, GPoint(ox + W/4, oy + row_y + 8), 22, theme_fg());
  char wind_buf[16];
  const char *wind_unit = (d->units == UNITS_METRIC) ? "KMH" : "MPH";
  snprintf(wind_buf, sizeof(wind_buf), "%d%s %s",
           d->wind_speed, wind_unit, d->wind_dir);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, wind_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(ox, oy + row_y + 16, W/2, 22),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Humidity / dew point (right column). User can toggle which one
  // appears via the Clay "Show dew point" switch.
  icon_draw_droplet(ctx, GPoint(ox + W*3/4, oy + row_y + 8), 18, theme_accent_blue());
  char hum_buf[8];
  if (d->use_dew_point) {
    snprintf(hum_buf, sizeof(hum_buf), "%d°", d->dew_point);
  } else {
    snprintf(hum_buf, sizeof(hum_buf), "%d%%", d->humidity);
  }
  graphics_draw_text(ctx, hum_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(ox + W/2, oy + row_y + 16, W/2, 22),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Second detail row: wind gusts | precipitation amount.
  // No icons — text only, smaller font, muted color.
  // Anchored 22px above banner top: H - pad_bottom(22/40) - banner_h(22) - 22.
  int row2_y = PBL_IF_ROUND_ELSE(H - 84, H - 66);
  // Single shared divider line for this row.
  graphics_context_set_stroke_color(ctx, theme_muted());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(ox + W/2, oy + row2_y), GPoint(ox + W/2, oy + row2_y + 18));

  // Left column: gust speed.
  char gust_buf[20];
  if (d->units == UNITS_METRIC) {
    snprintf(gust_buf, sizeof(gust_buf), "%dKMH GUSTS", d->wind_gust);
  } else {
    snprintf(gust_buf, sizeof(gust_buf), "%dMPH GUSTS", d->wind_gust);
  }
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, gust_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(ox, oy + row2_y + 2, W/2, 18),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Right column: precip amount. precip_amount is mm×10 from PKJS.
  char pa_buf[32];
  if (d->units == UNITS_IMPERIAL) {
    // mm×10 → tenths-of-inches: (mm×10 * 10) / 254 = mm×100 / 254
    int tenths_in = (d->precip_amount * 10) / 254;
    snprintf(pa_buf, sizeof(pa_buf), "%d.%dIN PRECIP", tenths_in / 10, tenths_in % 10);
  } else {
    snprintf(pa_buf, sizeof(pa_buf), "%d.%dMM PRECIP",
             d->precip_amount / 10, d->precip_amount % 10);
  }
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, pa_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(ox + W/2, oy + row2_y + 2, W/2, 18),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Rotating status banner (rain ⇄ updated).
  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      d->update_failed,
                      anim_get_frame());
}
