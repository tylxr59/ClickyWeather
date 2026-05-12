#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../settings.h"
#include <stdio.h>

// Phase 10D: Settings / Manage Cards card.
//
// Layout: header, 9 rows (2 LOCKED + 7 TOGGLEABLE), footer hint.
//
// Cursor model after Phase 10D:
//   - TAP screen     → advance cursor to next toggleable row.
//   - SELECT short   → toggle the highlighted row (no auto-advance).
//   - SELECT long    → app-wide theme toggle (handled in TouchWeather.c).
//   - UP / DOWN      → previous / next card (unchanged).
//
// Cursor chevron is drawn in the violet advice accent so it pops
// against fg labels and matches the Touch & Go card's identity color.

#define LOCKED_COUNT 2

static void prv_draw_checkbox(GContext *ctx, GRect r, bool checked) {
  graphics_context_set_stroke_color(ctx, theme_fg());
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_rect(ctx, r);
  if (checked) {
    graphics_context_set_fill_color(ctx, theme_fg());
    graphics_fill_rect(ctx, GRect(r.origin.x + 3, r.origin.y + 3,
                                  r.size.w - 6, r.size.h - 6),
                       0, GCornerNone);
  }
}

void card_settings_draw(GContext *ctx, GRect bounds) {
  int W = bounds.size.w;
  int H = bounds.size.h;

  ui_draw_card_header_with_icon(ctx, bounds,
      PBL_IF_ROUND_ELSE("MANAGE CARDS", "CARDS"),
      theme_fg(), UI_HEADER_Y, 18, icon_draw_settings_gear);

  GFont row_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  int row_h = PBL_IF_ROUND_ELSE(15, 14);
  int top_y = UI_HEADER_Y + UI_HEADER_HEIGHT + PBL_IF_ROUND_ELSE(8, 6);

  int box_size = 14;
  int gap = 10;
  int label_max_w = PBL_IF_ROUND_ELSE(130, 140);
  int row_w = label_max_w + gap + box_size;
  int row_x = bounds.origin.x + (W - row_w) / 2;
  int floor_x = bounds.origin.x + UI_MARGIN_X;
  if (row_x < floor_x) row_x = floor_x;

  int cursor = settings_cursor();
  const char *locked_labels[LOCKED_COUNT] = { "MAIN", "TOUCH & GO" };

  for (int i = 0; i < LOCKED_COUNT; ++i) {
    int y = top_y + i * row_h;
    graphics_context_set_text_color(ctx, theme_secondary());
    graphics_draw_text(ctx, locked_labels[i], row_font,
        GRect(row_x, y - 2, label_max_w, row_h + 2),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    icon_draw_lock_small(ctx,
        GPoint(row_x + label_max_w + gap + box_size/2, y + box_size/2),
        box_size, theme_secondary());
  }

  for (int i = 0; i < SETTINGS_TOGGLEABLE_COUNT; ++i) {
    int y = top_y + (LOCKED_COUNT + i) * row_h;
    bool is_cursor = (i == cursor);
    GColor txt = is_cursor ? theme_accent_advice() : theme_fg();

    // Cursor chevron.
    if (is_cursor) {
      graphics_context_set_fill_color(ctx, txt);
      // Triangle: filled chevron pointing right.
      GPathInfo info = (GPathInfo) {
        .num_points = 3,
        .points = (GPoint[]) {
          { row_x - 8, y + 2 },
          { row_x - 2, y + box_size/2 + 1 },
          { row_x - 8, y + box_size + 0 },
        }
      };
      GPath *p = gpath_create(&info);
      gpath_draw_filled(ctx, p);
      gpath_destroy(p);
    }

    graphics_context_set_text_color(ctx, txt);
    graphics_draw_text(ctx, settings_label((ToggleId)i), row_font,
        GRect(row_x, y - 2, label_max_w, row_h + 2),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    GRect box = GRect(row_x + label_max_w + gap, y + 1, box_size, box_size);
    prv_draw_checkbox(ctx, box,
                      settings_get_enabled((ToggleId)i));
  }

  // Footer hint: how to use this card. Sits above the page indicator.
  GFont hint_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  const char *hint = PBL_IF_ROUND_ELSE("TAP / SELECT",
                                       "TAP=MOVE  SELECT=TOGGLE");
  int hint_y = PBL_IF_ROUND_ELSE(H - 36, H - 32);
  graphics_context_set_text_color(ctx, theme_secondary());
  graphics_draw_text(ctx, hint, hint_font,
      GRect(bounds.origin.x, hint_y, W, 16),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}
