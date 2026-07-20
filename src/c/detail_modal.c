#include "detail_modal.h"
#include "theme.h"
#include "ui.h"
#include "icons.h"
#include "weather_data.h"
#include <stdio.h>

#define SLIDE_DURATION_MS 250
#define SLIDE_FRAME_MS 30
#define SHEET_HEIGHT_PCT 80
#define WEEK_DAYS 5

typedef enum { DM_IDLE = 0, DM_OPENING, DM_OPEN, DM_CLOSING } DMState;

static Layer *s_layer;
static AppTimer *s_slide_timer;
static DMState s_state;
static DetailType s_type;
static int s_screen_w;
static int s_screen_h;
static int s_sheet_h;
static int s_top_y;
static int s_slide_from;
static int s_slide_to;
static uint64_t s_slide_start_ms;
static bool s_pop_overlay;
static int s_week_page;

static uint64_t prv_now_ms(void) {
  time_t seconds;
  uint16_t millis;
  time_ms(&seconds, &millis);
  return (uint64_t)seconds * 1000ULL + millis;
}

static void prv_apply_frame(void) {
  if (!s_layer) return;
  layer_set_frame(s_layer, GRect(0, s_top_y, s_screen_w, s_sheet_h));
  layer_set_bounds(s_layer, GRect(0, 0, s_screen_w, s_sheet_h));
  layer_mark_dirty(s_layer);
}

static void prv_slide_tick(void *context) {
  (void)context;
  s_slide_timer = NULL;
  uint64_t elapsed = prv_now_ms() - s_slide_start_ms;
  if (elapsed >= SLIDE_DURATION_MS) {
    s_top_y = s_slide_to;
    prv_apply_frame();
    if (s_state == DM_OPENING) s_state = DM_OPEN;
    else if (s_state == DM_CLOSING) {
      s_state = DM_IDLE;
      s_type = DETAIL_NONE;
    }
    return;
  }
  int32_t progress = (int32_t)(elapsed * 1000 / SLIDE_DURATION_MS);
  int32_t inverse = 1000 - progress;
  int32_t eased = 1000 - inverse * inverse / 1000;
  s_top_y = s_slide_from + (s_slide_to - s_slide_from) * eased / 1000;
  prv_apply_frame();
  s_slide_timer = app_timer_register(SLIDE_FRAME_MS, prv_slide_tick, NULL);
}

static void prv_start_slide(int destination) {
  s_slide_from = s_top_y;
  s_slide_to = destination;
  s_slide_start_ms = prv_now_ms();
  if (s_slide_timer) app_timer_cancel(s_slide_timer);
  s_slide_timer = app_timer_register(SLIDE_FRAME_MS, prv_slide_tick, NULL);
}

static int prv_draw_chrome(GContext *ctx, const char *title,
                           UIIconDrawFn icon) {
  graphics_context_set_fill_color(ctx, theme_muted());
  graphics_fill_rect(ctx, GRect(s_screen_w / 2 - 14, 5, 28, 4),
                     2, GCornersAll);
  ui_draw_card_header_with_icon(ctx, GRect(0, 12, s_screen_w, UI_HEADER_HEIGHT),
                                title, theme_fg(), 0, 16, icon);
  return 12 + UI_HEADER_HEIGHT + 4;
}

static void prv_draw_hours(GContext *ctx) {
  WeatherData *data = weather_data_get();
  int top = prv_draw_chrome(ctx, "TEMP TREND", icon_draw_clock);
  graphics_context_set_text_color(ctx, theme_secondary());
  graphics_draw_text(ctx, s_pop_overlay ? "SELECT: hide rain %"
                                         : "SELECT: show rain %",
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, top, s_screen_w, 18),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  int margin = UI_MARGIN_X + 8;
  int chart_top = top + 34;
  int chart_bottom = s_sheet_h - 24;
  int chart_w = s_screen_w - 2 * margin;
  int chart_h = chart_bottom - chart_top;
  int minimum = data->hours_temp[0];
  int maximum = minimum;
  for (int i = 1; i < 6; ++i) {
    if (data->hours_temp[i] < minimum) minimum = data->hours_temp[i];
    if (data->hours_temp[i] > maximum) maximum = data->hours_temp[i];
  }
  int span = maximum - minimum;
  if (span < 1) span = 1;

  int px[6];
  int py[6];
  for (int i = 0; i < 6; ++i) {
    px[i] = margin + chart_w * i / 5;
    py[i] = chart_bottom - chart_h * (data->hours_temp[i] - minimum) / span;
  }
  ui_draw_dotted_hline(ctx, margin, margin + chart_w, chart_bottom + 2,
                       theme_muted());

  if (s_pop_overlay) {
    graphics_context_set_stroke_color(ctx, theme_accent_blue());
    graphics_context_set_fill_color(ctx, theme_accent_blue());
    graphics_context_set_stroke_width(ctx, 2);
    for (int i = 0; i < 6; ++i) {
      int y = chart_bottom - chart_h * data->hours_pop[i] / 100;
      if (i > 0) {
        int previous_y = chart_bottom - chart_h * data->hours_pop[i - 1] / 100;
        graphics_draw_line(ctx, GPoint(px[i - 1], previous_y), GPoint(px[i], y));
      }
      graphics_fill_circle(ctx, GPoint(px[i], y), 2);
    }
  }

  graphics_context_set_stroke_color(ctx, theme_accent_orange());
  graphics_context_set_fill_color(ctx, theme_accent_orange());
  graphics_context_set_stroke_width(ctx, 3);
  for (int i = 1; i < 6; ++i) {
    graphics_draw_line(ctx, GPoint(px[i - 1], py[i - 1]), GPoint(px[i], py[i]));
  }
  for (int i = 0; i < 6; ++i) {
    char value[8];
    snprintf(value, sizeof(value), "%d", data->hours_temp[i]);
    graphics_fill_circle(ctx, GPoint(px[i], py[i]), 3);
    graphics_context_set_text_color(ctx, theme_fg());
    graphics_draw_text(ctx, value, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(px[i] - 18, py[i] - 20, 36, 18),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_context_set_text_color(ctx, theme_secondary());
    graphics_draw_text(ctx, data->hours_label[i],
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(px[i] - 20, chart_bottom + 4, 40, 16),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

static void prv_draw_precip(GContext *ctx) {
  WeatherData *data = weather_data_get();
  int top = prv_draw_chrome(ctx, "RAINFALL", icon_draw_droplet);
  char headline[28];
  if (data->rain_alert_min >= 0) {
    int minutes = data->rain_alert_min;
    if (minutes < 60) snprintf(headline, sizeof(headline), "RAIN IN %dM", minutes);
    else snprintf(headline, sizeof(headline), "RAIN IN %dH", (minutes + 30) / 60);
  } else {
    snprintf(headline, sizeof(headline), "NO RAIN SOON");
    for (int i = 0; i < 6; ++i) {
      if (data->hours_pop[i] >= 40) {
        snprintf(headline, sizeof(headline), "%d%% CHANCE IN %dH",
                 data->hours_pop[i], i + 1);
        break;
      }
    }
  }
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, headline, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(0, top, s_screen_w, 22),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  int margin = UI_MARGIN_X + 8;
  int chart_top = top + 28;
  int chart_bottom = s_sheet_h - 38;
  int chart_w = s_screen_w - 2 * margin;
  int chart_h = chart_bottom - chart_top;
  int maximum = 1;
  for (int i = 0; i < 6; ++i) {
    if (data->hours_precip_x10[i] > maximum) maximum = data->hours_precip_x10[i];
  }
  int slot = chart_w / 6;
  int bar_w = slot - 6;
  ui_draw_dotted_hline(ctx, margin, margin + chart_w, chart_bottom, theme_muted());
  graphics_context_set_fill_color(ctx, theme_accent_blue());
  for (int i = 0; i < 6; ++i) {
    int height = chart_h * data->hours_precip_x10[i] / maximum;
    if (data->hours_precip_x10[i] > 0 && height < 2) height = 2;
    int x = margin + i * slot + (slot - bar_w) / 2;
    graphics_fill_rect(ctx, GRect(x, chart_bottom - height, bar_w, height),
                       1, GCornersTop);
    char pop[8];
    snprintf(pop, sizeof(pop), "%d%%", data->hours_pop[i]);
    graphics_context_set_text_color(ctx, theme_secondary());
    graphics_draw_text(ctx, pop, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(margin + i * slot, chart_bottom + 2, slot, 16),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, data->hours_label[i],
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(margin + i * slot, chart_bottom + 18, slot, 16),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

static GColor prv_condition_color(WeatherCondition condition) {
  if (condition == COND_SUNNY || condition == COND_PARTLY_CLOUDY)
    return theme_accent_orange();
  if (condition == COND_RAIN || condition == COND_SNOW || condition == COND_STORM)
    return theme_accent_blue();
  return theme_fg();
}

static void prv_draw_week(GContext *ctx) {
  WeatherData *data = weather_data_get();
  int page = s_week_page;
  int top = prv_draw_chrome(ctx, data->days_label[page], icon_draw_calendar);
  int center = s_screen_w / 2;
  icon_draw_condition(ctx, GPoint(center, top + 28), 42, data->days_cond[page]);

  char high[8];
  char low[8];
  snprintf(high, sizeof(high), "%d°", data->days_high[page]);
  snprintf(low, sizeof(low), "%d°", data->days_low[page]);
  graphics_context_set_text_color(ctx, theme_secondary());
  graphics_draw_text(ctx, "HIGH", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, top + 52, center, 16),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "LOW", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(center, top + 52, center, 16),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  graphics_context_set_text_color(ctx, prv_condition_color(data->days_cond[page]));
  graphics_draw_text(ctx, high, fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS),
                     GRect(0, top + 68, center, 32),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  graphics_context_set_text_color(ctx, theme_secondary());
  graphics_draw_text(ctx, low, fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS),
                     GRect(center, top + 68, center, 32),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  char pop[16];
  snprintf(pop, sizeof(pop), "%d%% RAIN", data->days_pop[page]);
  graphics_context_set_text_color(ctx, theme_accent_blue());
  graphics_draw_text(ctx, pop, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(0, top + 102, s_screen_w, 22),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  int start = center - 24;
  for (int i = 0; i < WEEK_DAYS; ++i) {
    GPoint dot = GPoint(start + i * 12, s_sheet_h - 10);
    if (i == page) {
      graphics_context_set_fill_color(ctx, theme_fg());
      graphics_fill_circle(ctx, dot, 3);
    } else {
      graphics_context_set_stroke_color(ctx, theme_muted());
      graphics_draw_circle(ctx, dot, 3);
    }
  }
}

static void prv_draw_uv(GContext *ctx) {
  WeatherData *data = weather_data_get();
  int top = prv_draw_chrome(ctx, "UV TODAY", icon_draw_sun);
  char summary[28];
  if (data->uv < 0) snprintf(summary, sizeof(summary), "UV ?  UNKNOWN");
  else snprintf(summary, sizeof(summary), "UV %d  %s", data->uv, uv_label(data->uv));
  graphics_context_set_text_color(ctx, theme_accent_orange());
  graphics_draw_text(ctx, summary, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(0, top, s_screen_w, 22),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  int margin = UI_MARGIN_X + 8;
  int chart_top = top + 34;
  int chart_bottom = s_sheet_h - 24;
  int chart_w = s_screen_w - 2 * margin;
  int chart_h = chart_bottom - chart_top;
  int maximum = 1;
  for (int i = 0; i < 6; ++i) if (data->hours_uv[i] > maximum) maximum = data->hours_uv[i];
  int previous_x = 0;
  int previous_y = 0;
  graphics_context_set_stroke_color(ctx, theme_accent_orange());
  graphics_context_set_fill_color(ctx, theme_accent_orange());
  graphics_context_set_stroke_width(ctx, 3);
  for (int i = 0; i < 6; ++i) {
    int x = margin + chart_w * i / 5;
    int value = data->hours_uv[i] < 0 ? 0 : data->hours_uv[i];
    int y = chart_bottom - chart_h * value / maximum;
    if (i > 0) graphics_draw_line(ctx, GPoint(previous_x, previous_y), GPoint(x, y));
    graphics_fill_circle(ctx, GPoint(x, y), 3);
    graphics_context_set_text_color(ctx, theme_secondary());
    graphics_draw_text(ctx, data->hours_label[i], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(x - 20, chart_bottom + 4, 40, 16),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    previous_x = x;
    previous_y = y;
  }
}

static GColor prv_aqi_color(int aqi) {
  if (aqi <= 50) return GColorIslamicGreen;
  if (aqi <= 100) return theme_accent_orange();
  if (aqi <= 150) return GColorOrange;
  if (aqi <= 200) return GColorRed;
  if (aqi <= 300) return GColorPurple;
  return GColorDarkCandyAppleRed;
}

static void prv_draw_aq(GContext *ctx) {
  WeatherData *data = weather_data_get();
  int top = prv_draw_chrome(ctx, "AIR DETAIL", icon_draw_pulse);
  GColor color = prv_aqi_color(data->aqi);
  char headline[28];
  snprintf(headline, sizeof(headline), "AQI %d  %s", data->aqi, aqi_label(data->aqi));
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, headline, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(0, top, s_screen_w, 22),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  const char *names[4] = { "PM2.5", "PM10", "O3", "NO2" };
  int values[4] = { data->pm2_5, data->pm10, data->o3, data->no2 };
  int maximum = 1;
  for (int i = 0; i < 4; ++i) if (values[i] > maximum) maximum = values[i];
  int margin = UI_MARGIN_X + 6;
  int y = top + 28;
  for (int i = 0; i < 4; ++i) {
    int row_y = y + i * 24;
    graphics_context_set_text_color(ctx, theme_secondary());
    graphics_draw_text(ctx, names[i], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(margin, row_y + 2, 45, 18),
                       GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    int bar_x = margin + 48;
    int bar_w = s_screen_w - bar_x - margin - 34;
    graphics_context_set_fill_color(ctx, theme_muted());
    graphics_fill_rect(ctx, GRect(bar_x, row_y + 7, bar_w, 8), 2, GCornersAll);
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, GRect(bar_x, row_y + 7, bar_w * values[i] / maximum, 8),
                       2, GCornersAll);
    char value[8];
    snprintf(value, sizeof(value), "%d", values[i]);
    graphics_context_set_text_color(ctx, theme_fg());
    graphics_draw_text(ctx, value, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(s_screen_w - margin - 32, row_y + 1, 32, 18),
                       GTextOverflowModeFill, GTextAlignmentRight, NULL);
  }
  graphics_context_set_text_color(ctx, theme_secondary());
  graphics_draw_text(ctx, "CONCENTRATIONS (ug/m3)",
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, y + 98, s_screen_w, 18),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void prv_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, theme_bg());
  graphics_fill_rect(ctx, bounds, 6, GCornerTopLeft | GCornerTopRight);
  graphics_context_set_stroke_color(ctx, theme_muted());
  graphics_draw_line(ctx, GPoint(0, 0), GPoint(bounds.size.w, 0));
  switch (s_type) {
    case DETAIL_HOURS: prv_draw_hours(ctx); break;
    case DETAIL_PRECIP: prv_draw_precip(ctx); break;
    case DETAIL_WEEK: prv_draw_week(ctx); break;
    case DETAIL_UV: prv_draw_uv(ctx); break;
    case DETAIL_AQ: prv_draw_aq(ctx); break;
    default: break;
  }
}

void detail_modal_init(Window *window) {
  GRect root = layer_get_bounds(window_get_root_layer(window));
  s_screen_w = root.size.w;
  s_screen_h = root.size.h;
  s_sheet_h = s_screen_h * SHEET_HEIGHT_PCT / 100;
  s_top_y = s_screen_h;
  s_state = DM_IDLE;
  s_type = DETAIL_NONE;
  s_layer = layer_create(GRect(0, s_top_y, s_screen_w, s_sheet_h));
  layer_set_clips(s_layer, true);
  layer_set_update_proc(s_layer, prv_update);
  layer_add_child(window_get_root_layer(window), s_layer);
}

void detail_modal_deinit(void) {
  if (s_slide_timer) {
    app_timer_cancel(s_slide_timer);
    s_slide_timer = NULL;
  }
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
  s_state = DM_IDLE;
}

bool detail_modal_is_active(void) { return s_state != DM_IDLE; }

bool detail_modal_open(DetailType type) {
  if (type == DETAIL_NONE || s_state != DM_IDLE) return false;
  s_type = type;
  s_pop_overlay = false;
  s_week_page = 0;
  s_state = DM_OPENING;
  s_top_y = s_screen_h;
  prv_start_slide(s_screen_h - s_sheet_h);
  return true;
}

void detail_modal_close(void) {
  if (s_state == DM_IDLE || s_state == DM_CLOSING) return;
  s_state = DM_CLOSING;
  prv_start_slide(s_screen_h);
}

bool detail_modal_handle_back(void) {
  if (!detail_modal_is_active()) return false;
  detail_modal_close();
  return true;
}

static void prv_page_week(int delta) {
  if (s_type != DETAIL_WEEK || s_state != DM_OPEN) return;
  int page = s_week_page + delta;
  if (page < 0) page = 0;
  if (page >= WEEK_DAYS) page = WEEK_DAYS - 1;
  if (page != s_week_page) {
    s_week_page = page;
    layer_mark_dirty(s_layer);
  }
}

bool detail_modal_handle_up(void) {
  prv_page_week(-1);
  return detail_modal_is_active();
}

bool detail_modal_handle_down(void) {
  prv_page_week(1);
  return detail_modal_is_active();
}

bool detail_modal_handle_select(void) {
  if (!detail_modal_is_active()) return false;
  if (s_state == DM_OPEN && s_type == DETAIL_HOURS) {
    s_pop_overlay = !s_pop_overlay;
    layer_mark_dirty(s_layer);
  }
  return true;
}
