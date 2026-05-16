import re

with open('src/c/cards/advice.c', 'r') as f:
    code = f.read()

# Make sure we add prv_generate_tier_headline above card_advice_draw
headline_func = """
// Phase 2: Dynamic Headline Generation
static void prv_generate_tier_headline(AdviceTier tier, WeatherData* d, char* buffer, size_t buf_size) {
  switch (tier) {
    case ADV_STORM:
      snprintf(buffer, buf_size, "STORM OVERHEAD");
      break;
    case ADV_RAIN_SOON:
      snprintf(buffer, buf_size, "RAIN IN %d MINS", d->rain_alert_min);
      break;
    case ADV_RAIN_NOW:
      snprintf(buffer, buf_size, "RAINING / %d°", d->temp);
      break;
    case ADV_SNOW:
      snprintf(buffer, buf_size, "SNOWING / %d°", d->temp);
      break;
    case ADV_HOT:
      snprintf(buffer, buf_size, "FEELS LIKE %d°", d->feels_like);
      break;
    case ADV_COLD:
      snprintf(buffer, buf_size, "FEELS LIKE %d°", d->feels_like);
      break;
    case ADV_WIND:
      snprintf(buffer, buf_size, "%d MPH GUSTS", d->wind_speed);
      break;
    case ADV_HIGH_UV:
      snprintf(buffer, buf_size, "UV INDEX: %d", d->uv);
      break;
    case ADV_BAD_AIR:
      snprintf(buffer, buf_size, "AQI: %d (POOR)", d->aqi);
      break;
    case ADV_MUGGY:
      snprintf(buffer, buf_size, "%d%% HUMIDITY", d->humidity);
      break;
    case ADV_PLEASANT:
    default:
      snprintf(buffer, buf_size, "%d° & CLEAR", d->temp);
      break;
  }
}

void card_advice_draw(GContext *ctx, GRect bounds) {
"""

code = code.replace("void card_advice_draw(GContext *ctx, GRect bounds) {", headline_func)

# Replace rendering logic
drawing_logic_old = """
  // Body rect: full content width minus 2*UI_MARGIN_X, vertically
  // centered between the badge bottom and the bottom safe zone (which
  // hosts the auto-banner + page indicator).
  int body_top = badge_y + 26;
  int body_bot = bounds.origin.y + bounds.size.h
                 - PBL_IF_ROUND_ELSE(70, 56);  // banner + indicator zone
  int body_h = body_bot - body_top;
  GRect body_r = GRect(bounds.origin.x + UI_MARGIN_X, body_top,
                       W - 2 * UI_MARGIN_X, body_h);
  if (!s_audited) { prv_audit_phrases(body_r); s_audited = true; }
  graphics_context_set_text_color(ctx, theme_fg());
  // Set font (18pt instead of 24pt might be necessary for bigger sentences, 
  // but we'll try 24-bold first and see if they fit)
  graphics_draw_text(ctx, phrase,
      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
      body_r, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
"""

drawing_logic_new = """
  // Body rect: dual layer layout
  int body_top = badge_y + 26;
  int body_bot = bounds.origin.y + bounds.size.h
                 - PBL_IF_ROUND_ELSE(70, 56);  // banner + indicator zone
  int body_h = body_bot - body_top;
  
  // Headline rect (top): ~22px height
  int headline_h = 22;
  GRect headline_r = GRect(bounds.origin.x + UI_MARGIN_X, body_top,
                           W - 2 * UI_MARGIN_X, headline_h);
                           
  char headline_text[32];
  prv_generate_tier_headline(tier, d, headline_text, sizeof(headline_text));
  
  // Draw the headline using the tier's accent color
  graphics_context_set_text_color(ctx, accent);
  graphics_draw_text(ctx, headline_text,
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      headline_r, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Quip rect (bottom): remaining height
  GRect quip_r = GRect(bounds.origin.x + UI_MARGIN_X, body_top + headline_h + 2,
                       W - 2 * UI_MARGIN_X, body_h - headline_h - 2);
                       
  if (!s_audited) { prv_audit_phrases(quip_r); s_audited = true; }
  graphics_context_set_text_color(ctx, theme_fg());
  graphics_draw_text(ctx, phrase,
      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
      quip_r, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
"""

code = code.replace(drawing_logic_old, drawing_logic_new)

with open('src/c/cards/advice.c', 'w') as f:
    f.write(code)

