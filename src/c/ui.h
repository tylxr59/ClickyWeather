#pragma once
#include <pebble.h>

// Symmetric horizontal margin used by every card so left and right
// padding always match. Tighter on rectangular displays where bezel
// is smaller, looser on round to avoid the corner curve.
#define UI_MARGIN_X PBL_IF_ROUND_ELSE(20, 12)

// Standardized header y-offset used by all cards. Tight on rect
// (8px from top) to reclaim vertical space; round needs more clearance
// for the bezel curve.
#define UI_HEADER_Y PBL_IF_ROUND_ELSE(24, 8)
#define UI_HEADER_HEIGHT 24

typedef enum {
  STATUS_BANNER_RAIN = 0,    // "RAIN IN <m>M" (orange)
  STATUS_BANNER_UPDATED = 1, // "UPDATED <x> AGO" (muted)
  STATUS_BANNER_FAILED = 2,  // "UPDATE FAILED" (orange)
} StatusBannerMode;

// Draws either a rain alert or a "last updated" pill at the bottom of
// `bounds`, reusing the same pill geometry. Returns true if drawn.
// minutes_to_rain: -1 for "no rain alert"; ignored when mode != RAIN.
// last_updated_secs: unix seconds of last refresh; 0 means "never".
bool ui_draw_status_banner(GContext *ctx, GRect bounds,
                           StatusBannerMode mode,
                           int minutes_to_rain,
                           uint32_t last_updated_secs);

// Convenience: pick a banner mode automatically given the data and
// current animation frame. If a rain alert is present we alternate
// every ~4s between rain and updated; otherwise we always show updated.
// frame is anim_get_frame() (~10 Hz).
bool ui_draw_auto_banner(GContext *ctx, GRect bounds,
                         int minutes_to_rain,
                         uint32_t last_updated_secs,
                         bool update_failed,
                         uint32_t frame);

// Centered uppercase header label. GOTHIC_18_BOLD across all cards.
void ui_draw_header(GContext *ctx, GRect bounds, const char *text,
                    GColor color, int y);

// Header with a small leading icon (cloud-rain, sun, pulse, ...).
// `draw_icon` receives the icon center point and `icon_size`.
typedef void (*UIIconDrawFn)(GContext *ctx, GPoint center, int size,
                             GColor color);
void ui_draw_card_header_with_icon(GContext *ctx, GRect bounds,
                                   const char *label, GColor color,
                                   int y, int icon_size,
                                   UIIconDrawFn draw_icon);

void ui_draw_dotted_hline(GContext *ctx, int x1, int x2, int y, GColor color);
