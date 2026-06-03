#include "anim.h"
#include "nav.h"
#include "weather_data.h"
#include "refresh_sheet.h"
#include <string.h>

#define ANIM_PERIOD_MS 100

static AppTimer *s_timer = NULL;
static uint32_t s_frame = 0;

static void prv_tick(void *ctx) {
  s_frame++;
  // Redraw if:
  //   - main card (animated hero icon), or
  //   - any card except sun-cycle when a rain alert is active
  //     (status banner rotates between RAIN and UPDATED every ~4s).
  int idx = nav_current_index();
  bool needs_redraw = (idx == 0);
  if (!needs_redraw) {
    WeatherData *d = weather_data_get();
    bool is_sun_cycle = (strcmp(nav_current_name(), "Sun Cycle") == 0);
    if (d->rain_alert_min >= 0 && !is_sun_cycle) {
      // Only mark dirty on banner-rotation boundaries (~every 4s).
      if ((s_frame % 40) == 0) needs_redraw = true;
    }
  }
  // Settings card has a rotating footer hint; refresh every ~2.5s.
  if (!needs_redraw && strcmp(nav_current_name(), "Settings") == 0) {
    if ((s_frame % 25) == 0) needs_redraw = true;
  }
  // Refresh sheet has its own animated indicator; keep ticking while
  // it's open so the spinner advances on every card.
  if (refresh_sheet_is_active()) needs_redraw = true;
  if (needs_redraw) {
    nav_redraw();
  }
  s_timer = app_timer_register(ANIM_PERIOD_MS, prv_tick, NULL);
}

void anim_init(void) {
  s_frame = 0;
  s_timer = app_timer_register(ANIM_PERIOD_MS, prv_tick, NULL);
}

void anim_deinit(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

uint32_t anim_get_frame(void) { return s_frame; }
