#include "cards.h"
#include "../theme.h"
#include "../icons.h"
#include "../ui.h"
#include "../weather_data.h"
#include "../anim.h"
#include <stdio.h>
#include <time.h>

// "Touch & Go" advice card.
//
// Classifies the current weather into one of N tiers, then picks a phrase
// from that tier's pool. Phrase selection is seeded by `last_updated` so
// the line stays stable for the duration of a refresh cycle (no flicker
// while the user stares at the card) and rotates organically when fresh
// weather data arrives.
//
// All phrases are stored locally — no remote calls, no persistence. The
// tier classifier is priority-ordered: the first matching tier wins.

typedef enum {
  ADV_STORM = 0,
  ADV_RAIN_SOON,
  ADV_RAIN_NOW,
  ADV_SNOW,
  ADV_HOT,
  ADV_COLD,
  ADV_WIND,
  ADV_HIGH_UV,
  ADV_BAD_AIR,
  ADV_MUGGY,
  ADV_PLEASANT,
  ADV_TIER_COUNT,
} AdviceTier;

typedef struct {
  const char *label;
  const char *const *phrases;
  int phrase_count;
} AdviceTierDef;

// ---- Phrase pools (~10 per tier). Practical-with-a-wink voice. ----

static const char *const PHRASES_STORM[] = {
  "Lightning out. Stay in. Don't be a conductor.",
  "Big sky drama. Indoor day.",
  "Storm rolling in. Charge phones, not the air.",
  "Thunder boss is busy. Reschedule outside plans.",
  "Sky's throwing a tantrum. Wait it out.",
  "Lightning loves tall things. Don't volunteer.",
  "It's a storm, not a vibe. Get inside.",
  "Cancel the picnic. Sky said no.",
  "Sky is loud. Go be quiet inside.",
  "Storm in progress. The sequel is worse.",
  "Sky's mad. Don't argue with it.",
  "Lightning likes umbrellas. Skip 'em.",
  "Thunder is sky's bad mood. Wait it out.",
  "Cancel everything outdoors. Trust me.",
  "Find a roof. Any roof.",
  "Storm tracker says: stay put.",
  "Power flicker possible. Charge now.",
  "Trees plus wind plus you = bad plan.",
  "Hail risk. Park the car under shelter.",
  "Don't be the tallest thing outside.",
  "Storms pass. Patience is free.",
  "Streets flood fast. Drive smart.",
  "Cellar weather. Pet, snacks, candle.",
  "Lightning crack count is miles. Count 'em.",
  "Big boom, small body. Stay inside.",
  "Hail's like sky pebbles. Hide.",
  "Skip the hike. Take the nap.",
  "Storm playlist on. Adulting off.",
  "Window seat, hot drink, no regrets.",
  "The sky's loud. Be quiet inside.",
};

static const char *const PHRASES_RAIN_SOON[] = {
  "Rain incoming. Last call for sunshine.",
  "Sky's about to leak. Move accordingly.",
  "Wet weather inbound. Grab the jacket.",
  "Heads up: clouds are loaded.",
  "Beat the rain or get soaked. Your call.",
  "Umbrella time soon. Don't be smug.",
  "Drops are queued. Prepare hood.",
  "Rain on the radar. The window's closing.",
  "Sky says: hurry, or get wet.",
  "Showers approaching. Make it quick.",
  "Window's closing. Move.",
  "Pop-up shower in 15-30 min. Heads up.",
  "Sky's loading. Grab the layer.",
  "Hood up or get dunked.",
  "Wet hair forecast: imminent.",
  "Walk fast. Or get wet. Choose.",
  "Errands? Now's the moment.",
  "Last dry minutes of the hour.",
  "Bring it, don't borrow it. Umbrella.",
  "Damp incoming. Shoes know.",
  "Rain on schedule. Stay sharp.",
  "Pavement's about to shine.",
  "Coffee run? Now or wet.",
  "Sky says: minutes left.",
  "Wrap the bag. Tuck the phone.",
  "Beat the cloud. It's on the way.",
  "Outdoor plans, last call.",
  "Rain timer running. Move.",
  "About to be a wet day. Soon.",
  "Soft drizzle warm-up coming.",
};

static const char *const PHRASES_RAIN_NOW[] = {
  "It's raining. Embrace it or escape it.",
  "Rain mode. Hood up.",
  "Wet outside. Don't pretend otherwise.",
  "Currently damp. Plan accordingly.",
  "It's coming down. So is your hair.",
  "Active drizzle. Boots > sneakers.",
  "Sky's washing the world. Dodge it.",
  "Rain is happening. Adjust expectations.",
  "Outside is wet. This is not news anymore.",
  "Take the umbrella. Yes, that one.",
  "Currently wet. Adjust footwear.",
  "Puddle world. Boots win.",
  "Rain. Coffee. Window. Repeat.",
  "Drops landing. So is your mood.",
  "Slick streets. Slower steps.",
  "Wet phone risk: high. Pocket it.",
  "Outside is wet. Inside is fine.",
  "Rain mode engaged. Plan around it.",
  "Sky's washing up. Stay out of it.",
  "Drizzle drama. Hood it.",
  "Mist or rain? Yes.",
  "Soggy shoes, sour mood. Avoid.",
  "Big drops, fast wets. Sprint.",
  "Steady rain. Just live with it.",
  "Petrichor party. Smell it.",
  "Sky's leaking. Step around it.",
  "Rain check accepted. Cozy in.",
  "Today's wet ad-lib. Roll with it.",
  "Coffee tastes better when it's wet.",
  "Indoor day, by sky decree.",
};

static const char *const PHRASES_SNOW[] = {
  "Snow falling. Layer up, walk slow.",
  "Slippery world. Mind the steps.",
  "It's snowing. Roads disagree with you.",
  "Cold and white out. Drive like grandma.",
  "Snow day energy. Hot drink mandatory.",
  "Boots, gloves, patience. In that order.",
  "Snow's doing its thing. So should you.",
  "Bundle up. The snow is not optional.",
  "Powder day. Or slush day. Probably slush.",
  "Soft sky. Sharp cold. Plan ahead.",
  "Snow's on. Drive slow, eat soup.",
  "Powder day. Boots ready.",
  "Frosty streets. Watch the curbs.",
  "Snow falls quiet. Roads don't.",
  "Heat the car, then the hands.",
  "Slush is the new ice. Beware.",
  "Hot drink mandatory. Today rule.",
  "Snow stacking. Boots aren't optional.",
  "Wipers up before parking. Trust me.",
  "Knit cap, full mitts, smug walk.",
  "Salt the steps. Save the spine.",
  "Sky's quiet, ground's loud.",
  "Layers beat willpower today.",
  "Sled weather. If a sled is near.",
  "Cold light, soft world.",
  "Snow days excuse everything.",
  "Drive like grandma. She's wise.",
  "Hot chocolate is medicine. Take it.",
  "Cold and white = take it slow.",
  "Bundle. Don't argue.",
};

static const char *const PHRASES_HOT[] = {
  "Hydrate or wilt.",
  "Cooking weather. Move with intent.",
  "It's hot. The car will be hotter.",
  "Sun's not playing. Find shade.",
  "Heat advisory: vibes only inside.",
  "Drink water. More than that.",
  "Sweat is the new accessory.",
  "Pavement is lava. Choose grass.",
  "Heat wave. Slow is fast today.",
  "Stay cool. Literally and figuratively.",
  "Shorts kind of day. Hydrate.",
  "Cool drink count: 3 minimum.",
  "AC is a love language.",
  "Skip the asphalt. Find shade.",
  "Heat warning: brains slow at 95.",
  "Dogs need shade. So do you.",
  "Iced everything. No exceptions.",
  "Sun's cooking. Stay raw.",
  "Pace yourself. Heat is patient.",
  "Wet rag on neck = secret weapon.",
  "Hot car warning. Crack a window.",
  "Sweat is hydration's receipt.",
  "Outdoor workout: dawn or dusk only.",
  "Heat dome, brain dome. Slow down.",
  "Move at low gear. The day's hot.",
  "Ice cubes are friends. The best ones.",
  "Wear less. Drink more. Win.",
  "Heat respects nobody. Don't test it.",
  "Pool beats pavement. Always today.",
  "Lemonade era. Embrace it.",
};

static const char *const PHRASES_COLD[] = {
  "Cover the extremities.",
  "Toe-numbing cold. Jacket non-negotiable.",
  "Layers are not a suggestion.",
  "It's cold. Your nose already knows.",
  "Hat now. Regret later avoided.",
  "Cold enough to taste. Bundle up.",
  "Brisk is generous. It's freezing.",
  "Hands in pockets, head down, go.",
  "Warm drink, warm coat, warm room.",
  "Winter said: prove you mean it.",
  "Bone-cold. Layers won. Gloves loud.",
  "Frost on windshield. Plan extra time.",
  "Two pairs of socks is wisdom.",
  "Wind chill bites harder than degrees.",
  "Hot bath waiting at home.",
  "Carry tea. Don't ask why.",
  "Hand warmers earn their keep.",
  "Lips chap, mood snaps. Balm up.",
  "Diesel weather. Move with intent.",
  "Snow boots, even if there's no snow.",
  "Cold like a memory. Sharp.",
  "Beard ice possible. Wear like a badge.",
  "Numb fingers = tea time.",
  "Wool socks fix 80% of cold.",
  "Engine's cold. Idle a minute.",
  "Step lighter on ice. Step shorter.",
  "Long underwear is not a punchline.",
  "Soup for lunch. Soup for dinner.",
  "Bundle isn't a vibe. It's a rule.",
  "Layers aren't optional today.",
};

static const char *const PHRASES_WIND[] = {
  "Hold your hat.",
  "Blustery. Tie down loose stuff.",
  "Wind's in charge today.",
  "Hair plans? Cancelled.",
  "Lean in. The wind certainly is.",
  "Outdoor papers will be airborne.",
  "Gusty. Mind the doors.",
  "Wind speaks. Listen, then duck.",
  "Heavy on the wind, light on patience.",
  "Hold the dog. Hold the snacks.",
  "Gusts strong enough to rearrange you.",
  "Door-slam day. Hold the handle.",
  "Garbage cans go for walks today.",
  "Hair gel earns its money. Or doesn't.",
  "Hat anchor required. Strings or chin.",
  "Bike ride? Pick the cross-wind route.",
  "Wind makes mild cold feel arctic.",
  "Trash bag flight risk. Tie it.",
  "Drive slow on highways. Crosswinds bite.",
  "Tarp anything outdoors.",
  "Watch the trees. They tell on the wind.",
  "Lean into it. Or get blown sideways.",
  "Wind chimes overtime today.",
  "Patio chairs become missiles.",
  "Wear weight today. Or be weight today.",
  "Skip the open field. Trust me.",
  "Tie down anything light. Now.",
  "Sunglasses double as eye armor.",
  "The wind is the boss. Obey.",
  "Hold the door for the next person.",
};

static const char *const PHRASES_HIGH_UV[] = {
  "Sunscreen isn't optional.",
  "Bright burn risk. Hat and shades.",
  "UV's loud today. Cover up.",
  "Sun is judging. So is your skin.",
  "Slip, slop, slap. Old advice, still true.",
  "Strong sun. Strong shade preference.",
  "Sunglasses earn their keep today.",
  "Reapply. Then reapply again.",
  "Skin remembers. Be kind to it.",
  "Bring shade with you if you must.",
  "UV 9+: burns in 15 minutes. Cover up.",
  "Sunscreen now. Reminder for later.",
  "Hat over hood. Wide brim wins.",
  "Shade is currency. Spend wisely.",
  "Reapply SPF every 80 minutes.",
  "UV bites silent. Cover before it does.",
  "Sunglasses are tools, not vibes.",
  "Beach? Bring SPF 50, then more.",
  "Long sleeves beat sunburn regret.",
  "UV index over 7 = serious.",
  "Future-you wants shade now.",
  "Lip balm with SPF. Trust me.",
  "Reapply after swim. Always.",
  "Even cloudy hours can burn.",
  "Walk on the shady side of the street.",
  "Hat-and-glasses is the look today.",
  "Sun's harsh. Be kind to your skin.",
  "Two layers of SPF beats one regret.",
  "Tan today, mole check tomorrow.",
  "Skin SPF is cheaper than dermatology.",
};

static const char *const PHRASES_BAD_AIR[] = {
  "Maybe skip the run.",
  "Air quality off. Indoor cardio wins.",
  "Lungs prefer indoors today.",
  "Air's having a rough one.",
  "Mask up if you're sensitive.",
  "Open windows? Maybe not today.",
  "Outdoor workout: postponed.",
  "Breathe shallow. Or just breathe inside.",
  "Air feels thick. Trust the gauge.",
  "Today is a 'stay in' kind of day.",
  "Air quality red. Skip the run.",
  "N95 if you're sensitive.",
  "Window closed, fan on, filter in.",
  "Defer outdoor exercise. Tomorrow's better.",
  "Air feels gritty. Trust the gauge.",
  "Asthma alert. Inhaler in pocket.",
  "Babies and seniors: indoors only.",
  "Smoke or smog? Yes.",
  "Brief errands only. Mask up.",
  "Indoor cardio is real cardio.",
  "Air purifier earns its keep today.",
  "Cancel the outdoor brunch.",
  "Even the dog should wait.",
  "Open windows? Maybe tomorrow.",
  "Eyes burning is the body's warning.",
  "Skip the long bike ride.",
  "Step out, step back. Quick only.",
  "Filter check: today's the day.",
  "Sensitive group? Stay in. Period.",
  "Lungs aren't optional. Be kind.",
};

static const char *const PHRASES_MUGGY[] = {
  "Sticky. Shower-after-walking territory.",
  "Humid soup. Lightweight clothes.",
  "Air is thick today. Move slow.",
  "Damp warmth. Shower's your friend.",
  "Hair has its own plans now.",
  "Cotton lies. Wear synthetics.",
  "Sweaty without effort. Welcome to summer.",
  "Mug season. Embrace the dewy look.",
  "Humidity says hi. Loudly.",
  "It's not the heat. Yes it is. And the wet.",
  "Damp warmth. Cotton lies.",
  "Humidity 75% feels like 110.",
  "Hair plans canceled by physics.",
  "Towel in the bag. Always.",
  "Sticky air. Move slow, drink more.",
  "Synthetic fabrics earn their keep.",
  "Sweat without effort. Welcome to muggy.",
  "Lightweight cotton beats tight denim.",
  "Hydrate like it's hot. It is.",
  "Frizz is a vibe today.",
  "Doorknobs feel damp. Wipe down.",
  "Tropical except for the beach.",
  "Dew point > air temp = soup.",
  "Lukewarm shower wins this round.",
  "Bring two shirts. You'll use both.",
  "Cotton sticks. Wear linen.",
  "Air feels chewable. Don't chew it.",
  "Sleep with the fan on. Don't ask.",
  "Glasses fog at every doorway.",
  "Tropical day. Tropical pace.",
};

static const char *const PHRASES_PLEASANT[] = {
  "Just nice out. Go enjoy something.",
  "Boring weather. The good kind.",
  "Outside is officially fine.",
  "Goldilocks day. Use it.",
  "No excuses. The weather is great.",
  "Touch grass weather.",
  "A walk would be wasted indoors.",
  "Nice out. Suspiciously nice. Go anyway.",
  "The kind of day weather apps brag about.",
  "All systems go. Step outside.",
  "Goldilocks day. Use every hour.",
  "Touch grass. The grass is ready.",
  "No excuse weather. Go do the thing.",
  "Sky's polite. Be polite back.",
  "Walk longer than planned.",
  "Picnic-grade conditions.",
  "Open the windows. Let the day in.",
  "Bike anywhere. Distance is forgiving.",
  "Errands feel like field trips today.",
  "Patio season activated.",
  "Coffee outside, not inside, today.",
  "Easy weather. Hard not to enjoy.",
  "Boring sky. Beautiful day.",
  "The kind of day to call someone.",
  "Linger outside. Nothing's chasing you.",
  "Open-window driving weather.",
  "Sunglasses, no jacket, no regrets.",
  "Easy on the bones, easy on the mood.",
  "A day without complaints.",
  "Friendly sky. Step into it.",
};

#define POOL(arr) (arr), (sizeof(arr) / sizeof((arr)[0]))

static const AdviceTierDef TIERS[ADV_TIER_COUNT] = {
  [ADV_STORM]     = { "STORM",     POOL(PHRASES_STORM) },
  [ADV_RAIN_SOON] = { "RAIN SOON", POOL(PHRASES_RAIN_SOON) },
  [ADV_RAIN_NOW]  = { "RAINY",     POOL(PHRASES_RAIN_NOW) },
  [ADV_SNOW]      = { "SNOW",      POOL(PHRASES_SNOW) },
  [ADV_HOT]       = { "HOT",       POOL(PHRASES_HOT) },
  [ADV_COLD]      = { "COLD",      POOL(PHRASES_COLD) },
  [ADV_WIND]      = { "WINDY",     POOL(PHRASES_WIND) },
  [ADV_HIGH_UV]   = { "HIGH UV",   POOL(PHRASES_HIGH_UV) },
  [ADV_BAD_AIR]   = { "BAD AIR",   POOL(PHRASES_BAD_AIR) },
  [ADV_MUGGY]     = { "MUGGY",     POOL(PHRASES_MUGGY) },
  [ADV_PLEASANT]  = { "NICE",      POOL(PHRASES_PLEASANT) },
};

#undef POOL

// One-shot audit: on first draw of this card we measure every phrase
// against the actual body rect on the live platform and log a warning
// if any wraps tall enough to clip. Authoring guideline is <= 55 chars
// (verified per-platform via the original probe pass on emery + gabbro;
// at 55 chars on emery the rendered block is ~72px tall vs a 110px
// body budget, leaving comfortable slack). Run is essentially free
// (one pass of ~330 layout calculations on first card draw).
static bool s_audited = false;

static void prv_audit_phrases(GRect body_r) {
  GFont f = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  for (int t = 0; t < ADV_TIER_COUNT; ++t) {
    const AdviceTierDef *def = &TIERS[t];
    for (int i = 0; i < def->phrase_count; ++i) {
      GSize s = graphics_text_layout_get_content_size(
          def->phrases[i], f,
          GRect(0, 0, body_r.size.w, 500),
          GTextOverflowModeWordWrap, GTextAlignmentCenter);
      if (s.h > body_r.size.h) {
        APP_LOG(APP_LOG_LEVEL_WARNING,
                "[advice] OVERFLOW tier=%d idx=%d h=%d>max=%d: \"%s\"",
                t, i, s.h, body_r.size.h, def->phrases[i]);
      }
    }
  }
}

// Classify weather into a tier. Priority-ordered: first match wins.
// Thresholds are imperial-leaning but work across both unit modes for the
// tongue-in-cheek output (a "95°" advice line on a 95°C day would be
// hyperbolic but still funny — and the unit context is implied by the
// other cards).
static AdviceTier prv_classify(const WeatherData *d) {
  if (d->condition == COND_STORM) return ADV_STORM;
  if (d->rain_alert_min >= 0 && d->rain_alert_min <= 30) return ADV_RAIN_SOON;
  if (d->condition == COND_RAIN) return ADV_RAIN_NOW;
  if (d->condition == COND_SNOW) return ADV_SNOW;
  if (d->feels_like >= 95) return ADV_HOT;
  if (d->feels_like <= 20) return ADV_COLD;
  if (d->wind_speed >= 20) return ADV_WIND;
  if (d->uv >= 8) return ADV_HIGH_UV;
  if (d->aqi >= 100) return ADV_BAD_AIR;
  if (d->humidity >= 75 && d->temp >= 70) return ADV_MUGGY;
  return ADV_PLEASANT;
}

// Tier-appropriate header icon.
static void prv_draw_tier_icon(GContext *ctx, GPoint c, int size,
                               GColor color, AdviceTier tier) {
  switch (tier) {
    case ADV_STORM:     icon_draw_lightning(ctx, c, size, color); break;
    case ADV_RAIN_SOON:
    case ADV_RAIN_NOW:  icon_draw_cloud_rain(ctx, c, size, color, color); break;
    case ADV_SNOW:      icon_draw_snow(ctx, c, size, color); break;
    case ADV_HOT:
    case ADV_HIGH_UV:
    case ADV_PLEASANT:  icon_draw_sun(ctx, c, size, color); break;
    case ADV_COLD:      icon_draw_snow(ctx, c, size, color); break;
    case ADV_WIND:      icon_draw_wind(ctx, c, size, color); break;
    case ADV_BAD_AIR:   icon_draw_pulse(ctx, c, size, color); break;
    case ADV_MUGGY:     icon_draw_droplet(ctx, c, size, color); break;
    default:            icon_draw_sun(ctx, c, size, color); break;
  }
}

// Tier-appropriate accent color for the badge below the header.
static GColor prv_tier_color(AdviceTier tier) {
  switch (tier) {
    case ADV_STORM:
    case ADV_HOT:
    case ADV_HIGH_UV:    return theme_accent_orange();
    case ADV_RAIN_SOON:
    case ADV_RAIN_NOW:
    case ADV_SNOW:
    case ADV_COLD:
    case ADV_BAD_AIR:
    case ADV_MUGGY:      return theme_accent_blue();
    case ADV_PLEASANT:   return theme_accent_advice();  // echo the card's identity color
    case ADV_WIND:
    default:             return theme_fg();
  }
}

void card_advice_draw(GContext *ctx, GRect bounds) {
  WeatherData *d = weather_data_get();
  int W = bounds.size.w;

  AdviceTier tier = prv_classify(d);
  const AdviceTierDef *def = &TIERS[tier];
  GColor accent = prv_tier_color(tier);

  // Header: distinct identity icon + color so this card never reads as
  // "Air Quality". Tap icon + mustard yellow are unique to Touch & Go.
  int header_y = UI_HEADER_Y;
  ui_draw_card_header_with_icon(ctx, bounds, "TOUCH & GO",
                                theme_fg(),
                                header_y, 18, icon_draw_tap);

  // Tier badge (icon + tier name) — small, right under the header.
  int badge_y = header_y + UI_HEADER_HEIGHT + PBL_IF_ROUND_ELSE(8, 4);
  GFont badge_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  GSize bsize = graphics_text_layout_get_content_size(def->label, badge_font,
      GRect(0, 0, W, 20), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
  int badge_icon = 16;
  int badge_gap = 5;
  int badge_total = badge_icon + badge_gap + bsize.w;
  int badge_x = bounds.origin.x + (W - badge_total) / 2;
  prv_draw_tier_icon(ctx, GPoint(badge_x + badge_icon/2, badge_y + 9),
                     badge_icon, accent, tier);
  graphics_context_set_text_color(ctx, accent);
  graphics_draw_text(ctx, def->label, badge_font,
      GRect(badge_x + badge_icon + badge_gap, badge_y, bsize.w + 4, 20),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Body phrase. Pick stable-per-refresh: seed by last_updated so the
  // same line shows for the entire session and refreshes when weather
  // data refreshes. Fall back to time-bucketed minute if never refreshed.
  uint32_t seed = d->last_updated ? d->last_updated
                                  : (uint32_t)(time(NULL) / 60);
  int idx = (int)(seed % (uint32_t)def->phrase_count);
  const char *phrase = def->phrases[idx];

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
  graphics_draw_text(ctx, phrase,
      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
      body_r, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Standard auto status banner so this card stays consistent with peers.
  ui_draw_auto_banner(ctx, bounds, d->rain_alert_min, d->last_updated,
                      anim_get_frame());
}
