#include "update_notes.h"
#include "theme.h"
#include "ui.h"
#include "version_gen.h"
#include <pebble.h>
#include <string.h>

#define PERSIST_KEY_NOTES_VERSION 400
#define MARKER_INDENT 14

static Window *s_window;
static Layer *s_header;
static ScrollLayer *s_scroll;
static Layer *s_body;

static int prv_layout_body(GContext *ctx, int width) {
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  int text_x = UI_MARGIN_X + MARKER_INDENT;
  int text_width = width - text_x - UI_MARGIN_X;
  static char notes[800];
  strncpy(notes, APP_UPDATE_NOTES, sizeof(notes) - 1);
  notes[sizeof(notes) - 1] = '\0';

  char *lines[20];
  int count = 1;
  lines[0] = notes;
  for (char *cursor = notes; *cursor && count < 20; ++cursor) {
    if (*cursor == '\n') {
      *cursor = '\0';
      lines[count++] = cursor + 1;
    }
  }

  int y = 6;
  for (int i = 0; i < count; ++i) {
    GSize size = graphics_text_layout_get_content_size(
        lines[i], font, GRect(0, 0, text_width, 1000),
        GTextOverflowModeWordWrap, GTextAlignmentLeft);
    if (ctx) {
      graphics_context_set_fill_color(ctx, theme_accent_orange());
      graphics_fill_circle(ctx, GPoint(UI_MARGIN_X + 4, y + 9), 3);
      graphics_context_set_text_color(ctx, theme_fg());
      graphics_draw_text(ctx, lines[i], font,
                         GRect(text_x, y, text_width, size.h + 6),
                         GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
    y += size.h + 10;
  }
  return y + 6;
}

static void prv_body_update(Layer *layer, GContext *ctx) {
  prv_layout_body(ctx, layer_get_bounds(layer).size.w);
}

static void prv_header_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, "WHAT'S NEW",
                     fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(0, 8, bounds.size.w, 30),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  char version[20] = "v" APP_VERSION_LABEL;
  graphics_context_set_text_color(ctx, theme_secondary());
  graphics_draw_text(ctx, version,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(0, 38, bounds.size.w, 18),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  ui_draw_dotted_hline(ctx, UI_MARGIN_X, bounds.size.w - UI_MARGIN_X, 58,
                       theme_muted());
}

static void prv_load(Window *window) {
  window_set_background_color(window, theme_bg());
  Layer *root = window_get_root_layer(window);
  GRect root_bounds = layer_get_bounds(root);
  int header_height = 64;

  s_header = layer_create(GRect(0, 0, root_bounds.size.w, header_height));
  layer_set_update_proc(s_header, prv_header_update);
  layer_add_child(root, s_header);

  s_scroll = scroll_layer_create(GRect(0, header_height, root_bounds.size.w,
                                       root_bounds.size.h - header_height));
  scroll_layer_set_shadow_hidden(s_scroll, true);
  scroll_layer_set_click_config_onto_window(s_scroll, window);
  int body_height = prv_layout_body(NULL, root_bounds.size.w);
  s_body = layer_create(GRect(0, 0, root_bounds.size.w, body_height));
  layer_set_update_proc(s_body, prv_body_update);
  scroll_layer_add_child(s_scroll, s_body);
  scroll_layer_set_content_size(s_scroll, GSize(root_bounds.size.w, body_height));
  layer_add_child(root, scroll_layer_get_layer(s_scroll));
}

static void prv_unload(Window *window) {
  (void)window;
  if (s_body) {
    layer_destroy(s_body);
    s_body = NULL;
  }
  if (s_scroll) {
    scroll_layer_destroy(s_scroll);
    s_scroll = NULL;
  }
  if (s_header) {
    layer_destroy(s_header);
    s_header = NULL;
  }
}

void update_notes_maybe_show(void) {
  int32_t seen = persist_exists(PERSIST_KEY_NOTES_VERSION)
      ? persist_read_int(PERSIST_KEY_NOTES_VERSION) : -1;
  if (seen == APP_VERSION_CODE) return;

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_load,
    .unload = prv_unload,
  });
  window_stack_push(s_window, true);
  persist_write_int(PERSIST_KEY_NOTES_VERSION, APP_VERSION_CODE);
}

void update_notes_deinit(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}
