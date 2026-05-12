#include "settings.h"
#include "theme.h"

// Persistence keys. Avoid collisions with comm.c PERSIST_KEY_CACHE=101.
#define KEY_THEME            200
#define KEY_TOGGLE_BASE      210  // KEY_TOGGLE_BASE + ToggleId

static bool s_enabled[SETTINGS_TOGGLEABLE_COUNT] = {
  true, true, true, true, true, true, true, true, true
};
static int s_cursor = 0;

static const char *s_labels[SETTINGS_TOGGLEABLE_COUNT] = {
  "6 HOURS", "WEEK AHEAD", "RAIN", "UV INDEX",
  "AIR QUAL", "SUN CYCLE", "NIGHT SKY", "GOLDEN HR", "RADAR",
};

void settings_load(void) {
  if (persist_exists(KEY_THEME)) {
    int t = persist_read_int(KEY_THEME);
    theme_set(t ? THEME_DARK : THEME_LIGHT);
  }
  for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
    if (persist_exists(KEY_TOGGLE_BASE + i)) {
      s_enabled[i] = persist_read_bool(KEY_TOGGLE_BASE + i);
    }
  }
}

void settings_save_theme(int theme_mode) {
  persist_write_int(KEY_THEME, theme_mode);
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

const char *settings_label(ToggleId id) {
  if (id >= SETTINGS_TOGGLEABLE_COUNT) return "";
  return s_labels[id];
}

int settings_cursor(void) { return s_cursor; }

void settings_cursor_advance(void) {
  s_cursor = (s_cursor + 1) % SETTINGS_TOGGLEABLE_COUNT;
}
