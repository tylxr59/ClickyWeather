#pragma once
#include <pebble.h>

#define NAV_MAX_CARDS 16

typedef void (*CardDrawFn)(GContext *ctx, GRect bounds);

typedef struct {
  const char *name;
  CardDrawFn draw;
} Card;

void nav_init(Window *window);
void nav_deinit(void);
void nav_register(const char *name, CardDrawFn draw);
int  nav_current_index(void);
const char *nav_current_name(void);
int  nav_count(void);
void nav_show_index(int idx);
void nav_next(void);
void nav_prev(void);
void nav_redraw(void);

// Phase 8: per-card enabled flag. A disabled card is skipped during
// nav_next/nav_prev and is hidden from the page indicator. Defaults to
// true for every card. The current card is never auto-disabled —
// callers should refresh after toggling so the indicator stays in sync.
void nav_set_enabled(int idx, bool enabled);
bool nav_is_enabled(int idx);
int  nav_count_enabled(void);
int  nav_active_enabled_index(void);

// Card visit order. Defaults to registration order. Callers may override
// with a permutation of card indices (length = nav_count()); nav_next /
// nav_prev / the page indicator all walk this order. Indices not present
// in the override are appended in registration order to keep the
// traversal complete.
void nav_set_traversal(const int *order, int count);

// Draws the 5-dot page indicator. Cards do NOT draw it; nav owns it.
// Cards may call this from within their draw fn if they want the
// indicator inside a custom layout — but by default nav layers it on top.
void nav_draw_page_indicator(GContext *ctx, GRect bounds, int active_index, int total);

// Returns true if a transition is currently animating. Callers can use
// this to drop rapid input; nav_next/nav_prev internally route through it.
bool nav_is_transitioning(void);
