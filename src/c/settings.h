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

bool settings_get_enabled(ToggleId id);
void settings_set_enabled(ToggleId id, bool enabled);
