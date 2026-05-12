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
  // Phase 10F slide transition: card draws into a bounds rect that may
  // be horizontally offset. Translate every X coordinate by ox so the
  // entire card moves as one rigid unit during the slide.
  int ox = bounds.origin.x;

  // Hero condition icon, centered, top.
  int icon_size = 48;
  int icon_y = PBL_IF_ROUND_ELSE(28, 22);
  icon_draw_condition_animated(ctx, GPoint(ox + W / 2, icon_y + icon_size/2),
                               icon_size, d->condition, anim_get_frame());

  // Big temperature "72°" — left-aligned at the margin so it doesn't
  // collide with the Hi/Lo column on the right. Symmetric margins on
  // both sides of the card; the *visible* text just takes whatever
  // width LECO_42 needs.
  char temp_buf[8];
  snprintf(temp_buf, sizeof(temp_buf), "%d°", d->temp);
  GFont temp_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  int temp_y = PBL_IF_ROUND_ELSE(60, 60);
  int temp_h = 50;
  int hilo_w = 52;
  GRect temp_r = GRect(bounds.origin.x + margin, temp_y,
                       W - 2 * margin - hilo_w, temp_h);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, temp_buf, temp_font, temp_r,
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);

  // Hi/Lo column on the right (symmetric margin from right edge).
  // Right-align the values so visible "64°/33°" hug W-margin, matching
  // the temp text on the left which hugs the left margin. This makes the
  // edge whitespace symmetric without changing the orientation/structure.
  int hilo_x = bounds.origin.x + W - margin - hilo_w;
  // Up arrow + "5°"
  icon_draw_arrow_up(ctx, GPoint(hilo_x, temp_y + 10), 10,
                     theme_accent_orange());
  char hi_buf[8]; snprintf(hi_buf, sizeof(hi_buf), "%d°", d->high);
  graphics_context_set_text_color(ctx, theme_accent_orange());
  graphics_draw_text(ctx, hi_buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(hilo_x + 10, temp_y + 2, hilo_w - 10, 22),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentRight, NULL);
  // Down arrow + "2°"
  icon_draw_arrow_down(ctx, GPoint(hilo_x, temp_y + 32), 10,
                       theme_accent_blue());
  char lo_buf[8]; snprintf(lo_buf, sizeof(lo_buf), "%d°", d->low);
  graphics_context_set_text_color(ctx, theme_accent_blue());
  graphics_draw_text(ctx, lo_buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(hilo_x + 10, temp_y + 24, hilo_w - 10, 22),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentRight, NULL);

  // "FEELS 75°" centered below big temp — bumped to 24_BOLD.
  char feels_buf[16];
  snprintf(feels_buf, sizeof(feels_buf), "FEELS %d°", d->feels_like);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, feels_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(ox, temp_y + temp_h, W, 30),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Bottom split row: wind | humidity. Sits above the rain banner.
  // emery moved 8px down to make room for the larger FEELS text.
  int row_y = PBL_IF_ROUND_ELSE(H - 104, H - 84);
  // Vertical divider.
  graphics_context_set_stroke_color(ctx, theme_muted());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(ox + W/2, row_y), GPoint(ox + W/2, row_y + 32));

  // Wind (left column).
  icon_draw_wind(ctx, GPoint(ox + W/4, row_y + 8), 22, theme_fg());
  char wind_buf[16];
  snprintf(wind_buf, sizeof(wind_buf), "%dMPH %s", d->wind_speed, d->wind_dir);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, wind_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(ox, row_y + 16, W/2, 22),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Humidity (right column).
  icon_draw_droplet(ctx, GPoint(ox + W*3/4, row_y + 8), 18, theme_accent_blue());
  char hum_buf[8];
  snprintf(hum_buf, sizeof(hum_buf), "%d%%", d->humidity);
  graphics_draw_text(ctx, hum_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(ox + W/2, row_y + 16, W/2, 22),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Rotating status banner (rain ⇄ updated).
  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      anim_get_frame());
}
