#include "settings.h"
#include "theme.h"

// Persistence keys. Avoid collisions with comm.c PERSIST_KEY_CACHE=101.
#define KEY_THEME            200
#define KEY_TOGGLE_BASE      210  // KEY_TOGGLE_BASE + ToggleId

// Visual order of rows in the Settings card. Decoupled from the enum
// order so Touch & Go appears second (after the locked MAIN row) without
// disrupting the persisted toggle keys (KEY_TOGGLE_BASE + ToggleId).
static const ToggleId s_visual_order[SETTINGS_TOGGLEABLE_COUNT] = {
  TOGGLE_ADVICE,  // "TOUCH & GO" — second row, right after MAIN
  TOGGLE_HOURS,
  TOGGLE_WEEK,
  TOGGLE_PRECIP,
  TOGGLE_UV,
  TOGGLE_AQ,
  TOGGLE_SUN,
  TOGGLE_NIGHT,
  TOGGLE_GOLDEN,
  TOGGLE_RADAR,
};

ToggleId settings_visual_id(int visual_pos) {
  if (visual_pos < 0 || visual_pos >= SETTINGS_TOGGLEABLE_COUNT) return TOGGLE_HOURS;
  return s_visual_order[visual_pos];
}

static bool s_enabled[SETTINGS_TOGGLEABLE_COUNT] = {
  true, true, true, true, true, true, true, true, true, true
};
static int s_cursor = 0;

static const char *s_labels[SETTINGS_TOGGLEABLE_COUNT] = {
  "6 HOURS", "WEEK AHEAD", "RAIN", "UV INDEX",
  "AIR QUAL", "SUN CYCLE", "NIGHT SKY", "GOLDEN HR", "RADAR",
  "TOUCH & GO",
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
