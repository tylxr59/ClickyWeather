#include "settings.h"
#include "theme.h"

// Persistence keys. Avoid collisions with comm.c PERSIST_KEY_CACHE=101.
#define KEY_THEME            200
#define KEY_TOGGLE_BASE      210  // KEY_TOGGLE_BASE + ToggleId
#define KEY_CARD_ORDER       220  // SETTINGS_TOGGLEABLE_COUNT bytes

// Default visual order of rows in the Settings card. Decoupled from
// the enum order so Touch & Go appears second (after the locked MAIN
// row) without disrupting the persisted toggle keys
// (KEY_TOGGLE_BASE + ToggleId). User reordering mutates s_visual_order
// in place; the defaults are restored only when the persisted order
// is missing or invalid.
static const uint8_t s_default_order[SETTINGS_TOGGLEABLE_COUNT] = {
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

static uint8_t s_visual_order[SETTINGS_TOGGLEABLE_COUNT];

static void prv_reset_order_to_default(void) {
  for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
    s_visual_order[i] = s_default_order[i];
  }
}

static bool prv_is_valid_permutation(const uint8_t *buf) {
  bool seen[SETTINGS_TOGGLEABLE_COUNT] = {0};
  for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
    uint8_t v = buf[i];
    if (v >= SETTINGS_TOGGLEABLE_COUNT) return false;
    if (seen[v]) return false;
    seen[v] = true;
  }
  return true;
}

static void prv_persist_order(void) {
  persist_write_data(KEY_CARD_ORDER, s_visual_order,
                     SETTINGS_TOGGLEABLE_COUNT);
}

ToggleId settings_visual_id(int visual_pos) {
  if (visual_pos < 0 || visual_pos >= SETTINGS_TOGGLEABLE_COUNT) return TOGGLE_HOURS;
  return (ToggleId)s_visual_order[visual_pos];
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
  prv_reset_order_to_default();
  if (persist_exists(KEY_CARD_ORDER)) {
    uint8_t buf[SETTINGS_TOGGLEABLE_COUNT];
    int n = persist_read_data(KEY_CARD_ORDER, buf, sizeof(buf));
    if (n == SETTINGS_TOGGLEABLE_COUNT && prv_is_valid_permutation(buf)) {
      for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
        s_visual_order[i] = buf[i];
      }
    }
  }
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

bool settings_move_up(int visual_pos) {
  if (visual_pos <= 0 || visual_pos >= SETTINGS_TOGGLEABLE_COUNT) return false;
  uint8_t tmp = s_visual_order[visual_pos - 1];
  s_visual_order[visual_pos - 1] = s_visual_order[visual_pos];
  s_visual_order[visual_pos] = tmp;
  s_cursor = visual_pos - 1;
  prv_persist_order();
  return true;
}

bool settings_move_down(int visual_pos) {
  if (visual_pos < 0 || visual_pos >= SETTINGS_TOGGLEABLE_COUNT - 1) return false;
  uint8_t tmp = s_visual_order[visual_pos + 1];
  s_visual_order[visual_pos + 1] = s_visual_order[visual_pos];
  s_visual_order[visual_pos] = tmp;
  s_cursor = visual_pos + 1;
  prv_persist_order();
  return true;
}
