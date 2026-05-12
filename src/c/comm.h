#pragma once
#include <pebble.h>

// Initialize AppMessage and request weather from PKJS.
void comm_init(void);
void comm_deinit(void);

// Ask PKJS to refresh now (e.g. on theme/units change or manual refresh).
void comm_request_refresh(void);

// Phase 12: Ask PKJS to fetch a fresh radar composite from the proxy
// and stream the pixel buffer back as RadarChunk* messages.
void comm_request_radar(void);

// Called whenever weather_data is updated from inbox (so UI can redraw).
typedef void (*CommUpdateCb)(void);
void comm_set_update_callback(CommUpdateCb cb);
