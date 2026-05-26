#include "settings.h"

// Persistence keys. Avoid collisions with comm.c PERSIST_KEY_CACHE=106.
#define KEY_TOGGLE_BASE      210  // KEY_TOGGLE_BASE + ToggleId

static bool s_enabled[SETTINGS_TOGGLEABLE_COUNT] = {
  true, true, true, true, true, true, true, true, true
};

void settings_load(void) {
  for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
    if (persist_exists(KEY_TOGGLE_BASE + i)) {
      s_enabled[i] = persist_read_bool(KEY_TOGGLE_BASE + i);
    }
  }
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
