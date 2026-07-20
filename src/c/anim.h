#pragma once
#include <pebble.h>

void anim_init(void);
void anim_deinit(void);
void anim_kick(void);
uint32_t anim_get_frame(void);
