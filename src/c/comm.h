#pragma once
#include <pebble.h>

// Initialize AppMessage and request weather from PKJS.
void comm_init(void);
void comm_deinit(void);

// Load cached weather data before first draw (prevents units flash).
void comm_load_cache(void);

// Ask PKJS to refresh now (e.g. startup, SELECT, or settings change).
void comm_request_refresh(void);

// Called whenever weather_data is updated from inbox (so UI can redraw).
typedef void (*CommUpdateCb)(void);
void comm_set_update_callback(CommUpdateCb cb);
