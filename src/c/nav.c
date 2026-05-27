#include "nav.h"
#include "theme.h"

static Card s_cards[NAV_MAX_CARDS];
static bool s_enabled[NAV_MAX_CARDS];
static int s_card_count = 0;
static int s_current = 0;

static Layer *s_card_layer = NULL;
static Layer *s_indicator_layer = NULL;

// Card transition state.
//
// When the user navigates between cards we run a 200ms vertical push
// slide. We DON'T spin up a new AppTimer for each transition;
// instead nav_tick_transition() is called from the existing 100ms
// anim ticker, plus we register a short-lived 30ms AppTimer for the
// duration of the slide so the motion stays smooth without bumping
// the global tick rate.
#define NAV_TRANSITION_DURATION_MS 200
#define NAV_TRANSITION_FRAME_MS    30

static bool s_anim_active = false;
static int  s_anim_from_idx = 0;
static int  s_anim_to_idx = 0;
static int  s_anim_dir = 0;          // +1 = next, -1 = prev
static uint64_t s_anim_start_ms = 0;
static AppTimer *s_anim_timer = NULL;

static uint64_t prv_now_ms(void) {
  time_t s; uint16_t ms;
  time_ms(&s, &ms);
  return (uint64_t)s * 1000ULL + (uint64_t)ms;
}

static void prv_anim_tick(void *ctx) {
  (void)ctx;
  s_anim_timer = NULL;
  if (!s_anim_active) return;
  uint64_t now = prv_now_ms();
  uint64_t elapsed = now - s_anim_start_ms;
  if (elapsed >= NAV_TRANSITION_DURATION_MS) {
    // Snap to final state.
    s_anim_active = false;
    s_current = s_anim_to_idx;
    if (s_card_layer) layer_mark_dirty(s_card_layer);
    if (s_indicator_layer) layer_mark_dirty(s_indicator_layer);
    return;
  }
  if (s_card_layer) layer_mark_dirty(s_card_layer);
  s_anim_timer = app_timer_register(NAV_TRANSITION_FRAME_MS,
                                    prv_anim_tick, NULL);
}

static void prv_start_transition(int new_idx, int dir) {
  if (s_anim_active) return;
  if (new_idx == s_current) return;
  s_anim_from_idx = s_current;
  s_anim_to_idx = new_idx;
  s_anim_dir = dir;
  s_anim_active = true;
  s_anim_start_ms = prv_now_ms();
  // Indicator jumps immediately to the new card so dot sync feels
  // tight; only the card-layer content slides.
  s_current = new_idx;
  if (s_indicator_layer) layer_mark_dirty(s_indicator_layer);
  if (s_card_layer) layer_mark_dirty(s_card_layer);
  if (s_anim_timer) app_timer_cancel(s_anim_timer);
  s_anim_timer = app_timer_register(NAV_TRANSITION_FRAME_MS,
                                    prv_anim_tick, NULL);
}

bool nav_is_transitioning(void) { return s_anim_active; }

static void card_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, theme_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (s_anim_active) {
    // Eased progress (ease-out quadratic): visible motion is fast at
    // the start and decelerates into the new card.
    uint64_t now = prv_now_ms();
    uint64_t elapsed = now - s_anim_start_ms;
    if (elapsed > NAV_TRANSITION_DURATION_MS) elapsed = NAV_TRANSITION_DURATION_MS;
    int32_t p1k = (int32_t)((elapsed * 1000) / NAV_TRANSITION_DURATION_MS); // 0..1000
    // ease-out: 1 - (1-p)^2  with fixed-point math.
    int32_t inv = 1000 - p1k;
    int32_t eased1k = 1000 - (inv * inv) / 1000;  // 0..1000

    int H = bounds.size.h;
    // shift: from card moves -dir*H*eased; to card moves from +dir*H to 0
    int from_dy = -s_anim_dir * (H * eased1k) / 1000;
    int to_dy   =  s_anim_dir * H + from_dy;

    GRect from_b = bounds; from_b.origin.y = bounds.origin.y + from_dy;
    GRect to_b   = bounds; to_b.origin.y   = bounds.origin.y + to_dy;

    if (s_anim_from_idx >= 0 && s_anim_from_idx < s_card_count &&
        s_cards[s_anim_from_idx].draw) {
      s_cards[s_anim_from_idx].draw(ctx, from_b);
    }
    if (s_anim_to_idx >= 0 && s_anim_to_idx < s_card_count &&
        s_cards[s_anim_to_idx].draw) {
      s_cards[s_anim_to_idx].draw(ctx, to_b);
    }
    return;
  }

  if (s_current >= 0 && s_current < s_card_count && s_cards[s_current].draw) {
    s_cards[s_current].draw(ctx, bounds);
  }
}

static void indicator_layer_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  // Build the enabled-only view: count = enabled cards, active = position
  // of current card within the enabled subset.
  nav_draw_page_indicator(ctx, b,
                          nav_active_enabled_index(),
                          nav_count_enabled());
}

void nav_draw_page_indicator(GContext *ctx, GRect bounds, int active_index, int total) {
  if (total <= 0) return;
  // Active dot = pill (16w x 4h, radius 2). Inactive = circle 4x4.
  const int dot_size = 4;
  const int active_w = 16;
  const int gap = 6;
  int total_w = total * dot_size + (total - 1) * gap + (active_w - dot_size);
  int x = bounds.origin.x + (bounds.size.w - total_w) / 2;
  int y = bounds.origin.y + (bounds.size.h - dot_size) / 2;

  for (int i = 0; i < total; ++i) {
    if (i == active_index) {
      GRect r = GRect(x, y, active_w, dot_size);
      graphics_context_set_fill_color(ctx, theme_indicator_active());
      graphics_fill_rect(ctx, r, 2, GCornersAll);
      x += active_w + gap;
    } else {
      GRect r = GRect(x, y, dot_size, dot_size);
      graphics_context_set_fill_color(ctx, theme_indicator_inactive());
      graphics_fill_rect(ctx, r, 2, GCornersAll);
      x += dot_size + gap;
    }
  }
}

void nav_init(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect rb = layer_get_bounds(root);

  // Card layer fills root.
  s_card_layer = layer_create(rb);
  layer_set_update_proc(s_card_layer, card_layer_update);
  layer_add_child(root, s_card_layer);

  // Page indicator overlay anchored near bottom safe-zone.
  // emery: 200x228 → y=210; gabbro round 260x260 → y=224 (inside circle).
  int indicator_y = PBL_IF_ROUND_ELSE(224, rb.size.h - 16);
  GRect ib = GRect(0, indicator_y, rb.size.w, 8);
  s_indicator_layer = layer_create(ib);
  layer_set_update_proc(s_indicator_layer, indicator_layer_update);
  layer_add_child(root, s_indicator_layer);
}

void nav_deinit(void) {
  if (s_anim_timer) { app_timer_cancel(s_anim_timer); s_anim_timer = NULL; }
  s_anim_active = false;
  if (s_card_layer) { layer_destroy(s_card_layer); s_card_layer = NULL; }
  if (s_indicator_layer) { layer_destroy(s_indicator_layer); s_indicator_layer = NULL; }
}

void nav_register(const char *name, CardDrawFn draw) {
  if (s_card_count >= NAV_MAX_CARDS) return;
  s_cards[s_card_count].name = name;
  s_cards[s_card_count].draw = draw;
  s_enabled[s_card_count] = true;
  s_card_count++;
}

int nav_current_index(void) { return s_current; }
int nav_count(void) { return s_card_count; }

const char *nav_current_name(void) {
  if (s_current < 0 || s_current >= s_card_count) return "";
  return s_cards[s_current].name ? s_cards[s_current].name : "";
}

void nav_show_index(int idx) {
  if (s_card_count == 0) return;
  // Wrap.
  while (idx < 0) idx += s_card_count;
  idx %= s_card_count;
  s_current = idx;
  // Cancel any in-flight transition — this is a hard jump.
  if (s_anim_timer) { app_timer_cancel(s_anim_timer); s_anim_timer = NULL; }
  s_anim_active = false;
  nav_redraw();
}

static int prv_step_skip(int from, int dir) {
  // Step from `from` in direction `dir` (+1 or -1), skipping disabled
  // cards. Always returns a valid index because at minimum Main is
  // always present.
  int idx = from;
  for (int i = 0; i < s_card_count; ++i) {
    idx += dir;
    while (idx < 0) idx += s_card_count;
    idx %= s_card_count;
    if (s_enabled[idx]) return idx;
  }
  return from;
}

void nav_next(void) {
  if (s_anim_active) return;
  int dst = prv_step_skip(s_current, +1);
  if (dst == s_current) return;
  prv_start_transition(dst, +1);
}
void nav_prev(void) {
  if (s_anim_active) return;
  int dst = prv_step_skip(s_current, -1);
  if (dst == s_current) return;
  prv_start_transition(dst, -1);
}

void nav_redraw(void) {
  if (s_card_layer) layer_mark_dirty(s_card_layer);
  if (s_indicator_layer) layer_mark_dirty(s_indicator_layer);
}

void nav_set_enabled(int idx, bool enabled) {
  if (idx < 0 || idx >= s_card_count) return;
  s_enabled[idx] = enabled;
  nav_redraw();
}

bool nav_is_enabled(int idx) {
  if (idx < 0 || idx >= s_card_count) return false;
  return s_enabled[idx];
}

int nav_count_enabled(void) {
  int n = 0;
  for (int i = 0; i < s_card_count; ++i) if (s_enabled[i]) n++;
  return n;
}

int nav_active_enabled_index(void) {
  int n = 0;
  for (int i = 0; i < s_card_count; ++i) {
    if (i == s_current) return n;
    if (s_enabled[i]) n++;
  }
  return 0;
}
