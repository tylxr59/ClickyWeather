#pragma once
#include <pebble.h>

void card_main_draw(GContext *ctx, GRect bounds);
void card_hours_draw(GContext *ctx, GRect bounds);
void card_week_draw(GContext *ctx, GRect bounds);
void card_precipitation_draw(GContext *ctx, GRect bounds);
void card_uv_draw(GContext *ctx, GRect bounds);
void card_air_quality_draw(GContext *ctx, GRect bounds);
void card_sun_cycle_draw(GContext *ctx, GRect bounds);
void card_night_sky_draw(GContext *ctx, GRect bounds);
void card_golden_hour_draw(GContext *ctx, GRect bounds);
void card_alerts_draw(GContext *ctx, GRect bounds);
