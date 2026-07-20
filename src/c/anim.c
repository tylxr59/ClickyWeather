#include "anim.h"
#include "nav.h"
#include "weather_data.h"
#include "settings.h"
#include <string.h>

#define ANIM_PERIOD_MS 100
#define ANIM_TIMEOUT_FRAMES 80

static AppTimer *s_timer = NULL;
static uint32_t s_frame = 0;
static uint32_t s_deadline_frame = 0;

static void prv_tick(void *ctx);

static bool prv_decorative_active(void) {
  return settings_get_animations_enabled() && s_frame < s_deadline_frame;
}

static void prv_ensure_running(void) {
  if (!s_timer) {
    s_timer = app_timer_register(ANIM_PERIOD_MS, prv_tick, NULL);
  }
}

static void prv_tick(void *ctx) {
  (void)ctx;
  s_timer = NULL;
  s_frame++;
  // Redraw if:
  //   - main card (animated hero icon), or
  //   - any card except sun-cycle when a rain alert is active
  //     (status banner rotates between RAIN and UPDATED every ~4s).
  bool needs_redraw = false;
  if (prv_decorative_active()) {
    int idx = nav_current_index();
    needs_redraw = (idx == 0);
    if (!needs_redraw) {
      WeatherData *d = weather_data_get();
      bool is_sun_cycle = (strcmp(nav_current_name(), "Sun Cycle") == 0);
      if ((d->rain_alert_min >= 0 || d->update_available) && !is_sun_cycle) {
        if ((s_frame % 40) == 0) needs_redraw = true;
      }
    }
  }
  if (needs_redraw) {
    nav_redraw();
  }
  if (prv_decorative_active()) {
    s_timer = app_timer_register(ANIM_PERIOD_MS, prv_tick, NULL);
  }
}

void anim_init(void) {
  s_frame = 0;
  s_deadline_frame = ANIM_TIMEOUT_FRAMES;
  s_timer = app_timer_register(ANIM_PERIOD_MS, prv_tick, NULL);
}

void anim_kick(void) {
  s_deadline_frame = s_frame + ANIM_TIMEOUT_FRAMES;
  prv_ensure_running();
}

void anim_deinit(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

uint32_t anim_get_frame(void) { return s_frame; }
