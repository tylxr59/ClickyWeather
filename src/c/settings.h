#pragma once
#include <pebble.h>

// Persisted user preferences. Card visibility + theme survive
// across app close/reopen via persist_*.
//
// The first card (index 0 = Main) is PERMANENT. It is not represented
// in this module; callers should always treat it as enabled.
//
// The Settings card itself (last in the carousel) is also always
// enabled — there'd be no escape hatch otherwise. It is NOT in the
// toggleable list.

// Number of cards that can be toggled on the Settings screen.
#define SETTINGS_TOGGLEABLE_COUNT 10

// Toggleable card identifiers (must match the registration order in
// TouchWeather.c skipping Main and Settings).
//
// TOGGLE_ADVICE was added last (rather than at the top of the enum)
// so existing persisted toggle values at KEY_TOGGLE_BASE + 0..8 keep
// their meaning across upgrades. Visual order in the settings card
// follows this enum order.
typedef enum {
  TOGGLE_HOURS = 0,
  TOGGLE_WEEK,
  TOGGLE_PRECIP,
  TOGGLE_UV,
  TOGGLE_AQ,
  TOGGLE_SUN,
  TOGGLE_NIGHT,
  TOGGLE_GOLDEN,
  TOGGLE_RADAR,
  TOGGLE_ADVICE,
} ToggleId;

void settings_load(void);
void settings_save_theme(int theme_mode);

bool settings_get_enabled(ToggleId id);
void settings_set_enabled(ToggleId id, bool enabled);

// Returns the display label for a toggleable card.
const char *settings_label(ToggleId id);

// Cursor on the Settings screen. Wraps within toggleable rows.
int  settings_cursor(void);
void settings_cursor_advance(void);

// Maps a visual row position (0 = first toggleable row) to the
// ToggleId that should be drawn/toggled there. Decouples the on-screen
// order from the enum order (which is fixed for persistence compat).
ToggleId settings_visual_id(int visual_pos);
