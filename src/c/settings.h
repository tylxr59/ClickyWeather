#pragma once
#include <pebble.h>

// Persisted user preferences. Card visibility survives across app
// close/reopen via persist_*.
//
// The first card (index 0 = Main) is PERMANENT. It is not represented
// in this module; callers should always treat it as enabled.
//
// Number of cards that can be toggled from the phone settings page.
#define SETTINGS_TOGGLEABLE_COUNT 9

// Toggleable card identifiers (must match the registration order in
// ClickyWeather.c skipping Main).
typedef enum {
  TOGGLE_HOURS = 0,
  TOGGLE_WEEK,
  TOGGLE_PRECIP,
  TOGGLE_UV,
  TOGGLE_AQ,
  TOGGLE_SUN,
  TOGGLE_NIGHT,
  TOGGLE_GOLDEN,
  TOGGLE_ALERTS,   // appended last to preserve all existing persist keys
} ToggleId;

void settings_load(void);

// Card-navigation loop preference. When true (default), pressing UP on the
// first card or DOWN on the last card wraps around the carousel. When false,
// reaching either boundary and stepping past it exits the app (Quick Launch
// style). Persisted across launches.
bool settings_get_loop_nav(void);
void settings_set_loop_nav(bool loop);

// Background update interval in seconds. Zero disables wakeup updates.
int settings_get_background_interval(void);
void settings_set_background_interval(int interval_secs);

// Decorative motion is enabled by default but settles after a short idle
// window. Disabling it makes the hero icon and rotating banners static.
bool settings_get_animations_enabled(void);
void settings_set_animations_enabled(bool enabled);

bool settings_get_enabled(ToggleId id);
void settings_set_enabled(ToggleId id, bool enabled);
