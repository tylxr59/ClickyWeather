#pragma once
#include <pebble.h>

typedef enum {
  DETAIL_NONE = 0,
  DETAIL_HOURS,
  DETAIL_PRECIP,
  DETAIL_WEEK,
  DETAIL_UV,
  DETAIL_AQ,
} DetailType;

void detail_modal_init(Window *window);
void detail_modal_deinit(void);
bool detail_modal_is_active(void);
bool detail_modal_open(DetailType type);
void detail_modal_close(void);
bool detail_modal_handle_back(void);
bool detail_modal_handle_up(void);
bool detail_modal_handle_down(void);
bool detail_modal_handle_select(void);
