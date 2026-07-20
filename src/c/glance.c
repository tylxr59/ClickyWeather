#include "glance.h"
#include "weather_data.h"
#include <pebble.h>
#include <stdio.h>

#define GLANCE_TTL_SECS (3 * 60 * 60)

static const char *const s_condition_labels[] = {
  "Sunny", "Partly Cloudy", "Cloudy", "Rain", "Snow", "Storm", "Fog"
};
static char s_subtitle[32];
static time_t s_expiration;

static void prv_reload(AppGlanceReloadSession *session, size_t limit,
                       void *context) {
  (void)context;
  if (limit < 1) return;
  const AppGlanceSlice slice = {
    .layout = {
      .icon = APP_GLANCE_SLICE_DEFAULT_ICON,
      .subtitle_template_string = s_subtitle,
    },
    .expiration_time = s_expiration,
  };
  app_glance_add_slice(session, slice);
}

void glance_update(void) {
  WeatherData *data = weather_data_get();
  if (!data->valid || data->last_updated == 0) return;
  s_expiration = (time_t)data->last_updated + GLANCE_TTL_SECS;
  if (s_expiration <= time(NULL) + 60) return;

  int condition = (int)data->condition;
  const char *label = condition >= 0 && condition < 7
      ? s_condition_labels[condition] : "";
  snprintf(s_subtitle, sizeof(s_subtitle), "%d° %s", data->temp, label);
  app_glance_reload(prv_reload, NULL);
}
