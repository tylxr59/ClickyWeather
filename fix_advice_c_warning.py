import re

with open('src/c/cards/advice.c', 'r') as f:
    code = f.read()

# Replace drawing logic to use the returned string from prv_generate_tier_headline. Oh wait, it IS used: 
# prv_generate_tier_headline(tier, d, headline_text, sizeof(headline_text));
# Wait, I'm getting "prv_generate_tier_headline defined but not used", this means my previous replacement placed it *after* where it was used, or something?
# No, let's just make sure it's uncommented properly and placed at the top.

# Ah, let's replace the whole method section to be clean.
c_new = """static void prv_generate_tier_headline(AdviceTier tier, WeatherData* d, char* buffer, size_t buf_size) {
  switch (tier) {
    case ADV_STORM:      snprintf(buffer, buf_size, "STORM OVERHEAD"); break;
    case ADV_RAIN_SOON:  snprintf(buffer, buf_size, "RAIN IN %d MINS", d->rain_alert_min); break;
    case ADV_RAIN_NOW:   snprintf(buffer, buf_size, "RAINING / %d°", d->temp); break;
    case ADV_SNOW:       snprintf(buffer, buf_size, "SNOWING / %d°", d->temp); break;
    case ADV_HOT:        snprintf(buffer, buf_size, "FEELS LIKE %d°", d->feels_like); break;
    case ADV_COLD:       snprintf(buffer, buf_size, "FEELS LIKE %d°", d->feels_like); break;
    case ADV_WIND:       snprintf(buffer, buf_size, "%d MPH GUSTS", d->wind_speed); break;
    case ADV_HIGH_UV:    snprintf(buffer, buf_size, "UV INDEX: %d", d->uv); break;
    case ADV_BAD_AIR:    snprintf(buffer, buf_size, "AQI: %d (POOR)", d->aqi); break;
    case ADV_MUGGY:      snprintf(buffer, buf_size, "%d%% HUMIDITY", d->humidity); break;
    case ADV_PLEASANT:
    default:             snprintf(buffer, buf_size, "%d° & CLEAR", d->temp); break;
  }
}
void card_advice_draw(GContext *ctx, GRect bounds) {"""

code = re.sub(r'static void prv_generate_tier_headline.*?\nvoid card_advice_draw\(GContext \*ctx,\s*GRect bounds\) \{', c_new, code, flags=re.DOTALL)

with open('src/c/cards/advice.c', 'w') as f:
    f.write(code)

