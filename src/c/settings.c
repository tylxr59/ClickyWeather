#include "settings.h"

// Persistence keys. Avoid collisions with comm.c PERSIST_KEY_CACHE.
#define KEY_LOOP_NAV         201  // bool: wrap card carousel at edges
#define KEY_BG_UPDATE_INTERVAL 202
#define KEY_TOGGLE_BASE      210  // KEY_TOGGLE_BASE + ToggleId

// Default: loop the carousel (wrap at the first/last card).
static bool s_loop_nav = true;
static int s_bg_update_interval = 0;

static bool s_enabled[SETTINGS_TOGGLEABLE_COUNT] = {
  true, true, true, true, true, true, true, true, true
};

void settings_load(void) {
  for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
    if (persist_exists(KEY_TOGGLE_BASE + i)) {
      s_enabled[i] = persist_read_bool(KEY_TOGGLE_BASE + i);
    }
  }
  if (persist_exists(KEY_LOOP_NAV)) {
    s_loop_nav = persist_read_bool(KEY_LOOP_NAV);
  }
  if (persist_exists(KEY_BG_UPDATE_INTERVAL)) {
    int interval = persist_read_int(KEY_BG_UPDATE_INTERVAL);
    if (interval == 0 || (interval >= 300 && interval <= 86400)) {
      s_bg_update_interval = interval;
    } else {
      s_bg_update_interval = 0;
      persist_write_int(KEY_BG_UPDATE_INTERVAL, 0);
    }
  }
}

bool settings_get_loop_nav(void) {
  return s_loop_nav;
}

void settings_set_loop_nav(bool loop) {
  s_loop_nav = loop;
  persist_write_bool(KEY_LOOP_NAV, loop);
}

int settings_get_background_interval(void) {
  return s_bg_update_interval;
}

void settings_set_background_interval(int interval_secs) {
  s_bg_update_interval = interval_secs;
  persist_write_int(KEY_BG_UPDATE_INTERVAL, interval_secs);
}

bool settings_get_enabled(ToggleId id) {
  if (id >= SETTINGS_TOGGLEABLE_COUNT) return true;
  return s_enabled[id];
}

void settings_set_enabled(ToggleId id, bool enabled) {
  if (id >= SETTINGS_TOGGLEABLE_COUNT) return;
  s_enabled[id] = enabled;
  persist_write_bool(KEY_TOGGLE_BASE + id, enabled);
}
