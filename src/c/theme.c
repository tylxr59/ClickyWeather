#include "theme.h"

#define PERSIST_KEY_THEME 1

static ThemeMode s_mode = THEME_LIGHT;
static Window *s_window = NULL;

void theme_init(void) {
  if (persist_exists(PERSIST_KEY_THEME)) {
    s_mode = (ThemeMode)persist_read_int(PERSIST_KEY_THEME);
  } else {
    s_mode = THEME_LIGHT;
  }
}

void theme_set(ThemeMode mode) {
  s_mode = mode;
  persist_write_int(PERSIST_KEY_THEME, (int)mode);
  // Auto-apply to bound window so callers don't have to remember.
  if (s_window) window_set_background_color(s_window, theme_bg());
}

ThemeMode theme_get(void) { return s_mode; }

GColor theme_bg(void) {
  return s_mode == THEME_DARK ? GColorBlack : GColorWhite;
}

GColor theme_fg(void) {
  return s_mode == THEME_DARK ? GColorWhite : GColorBlack;
}

GColor theme_muted(void) {
  return s_mode == THEME_DARK ? GColorDarkGray : GColorLightGray;
}

// Secondary body-text color: darker on light bg, lighter on dark bg.
// `theme_muted` looks fine on tracks/dividers but its light-mode value
// (LightGray on white) is unreadable for actual text. Use this for any
// body text that needs to be de-emphasized but still legible.
GColor theme_secondary(void) {
  return s_mode == THEME_DARK ? GColorLightGray : GColorDarkGray;
}

GColor theme_accent_orange(void) {
  // ~#FFAA00 — Pebble's chrome yellow / orange
  return GColorChromeYellow;
}

GColor theme_accent_blue(void) {
  // ~#00AAFF — bright cyan/blue
  return GColorVividCerulean;
}

GColor theme_accent_advice(void) {
  // Purple (GColorVividViolet) — distinct from orange (sunrise/UV/banner)
  // and blue (sunset/precip/AQ). Readable on both dark and light backgrounds.
  // On 1-bit displays Pebble auto-falls back to white.
  return GColorVividViolet;
}

GColor theme_indicator_active(void) { return theme_fg(); }
GColor theme_indicator_inactive(void) { return theme_muted(); }

void theme_apply_to_window(Window *window) {
  s_window = window;
  if (window) window_set_background_color(window, theme_bg());
}
