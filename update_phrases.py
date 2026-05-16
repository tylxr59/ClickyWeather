import re

with open('src/c/cards/advice.c', 'r') as f:
    code = f.read()

# I will provide completely hand-crafted phrases per category.
new_storms = """static const char *const PHRASES_STORM[] = {
  "Stay off the roof today. Nature is throwing sparks.",
  "Cancel the picnic. The sky said no.",
  "Big booms inbound. Small bodies stay inside.",
  "The sequel is worse. Stay off the golf course.",
  "If you can hear it, you can feel it. Get inside.",
  "Thunder boss is busy. Grab a good book.",
  "Active storm. Don't be a human lightning rod.",
  "Heavy skies overhead. Find a solid roof.",
  "Lightning is present. Reschedule outdoor plans.",
  "The sky is throwing a tantrum. Wait it out.",
  "Electrical storm. Put the umbrella down.",
  "Sky is flashing. Best watch from the couch.",
  "Nature's light show is active. Seek shelter.",
  "Loud and bright outside. Stay inside and stay dry.",
  "Thunderstorms ahead. Today is an indoor kind of day."
};"""

new_rain_soon = """static const char *const PHRASES_RAIN_SOON[] = {
  "Window's closing. Run for the canopy.",
  "Drops are literally queued. Beat the countdown.",
  "Last call for sunshine. Move accordingly.",
  "Grab the jacket. Wet weather is inbound.",
  "Pop-up shower coming. Pavement will shine soon.",
  "Heads up, clouds are loaded. Beat the rain.",
  "Sky says hurry. Or get wet, your choice.",
  "Damp incoming. Last dry minutes of the hour.",
  "Prepare the hood. The sky is about to leak.",
  "Incoming shower. Get to cover while you can.",
  "Rain is imminent. Fast walk recommended.",
  "The grace period is over. Find an umbrella.",
  "Rain preview starting now. Main event soon.",
  "Dry time expires shortly. Plan your escape.",
  "It's about to get wet. Save your suede."
};"""

new_rain_now = """static const char *const PHRASES_RAIN_NOW[] = {
  "It is pouring. Adjust your expectations.",
  "Coffee tastes better when the outside is completely soaked.",
  "Slick streets ahead. Dodge it if you can.",
  "Active drizzle. Boots are greater than sneakers.",
  "Wet phone risk: high. Pocket it safely.",
  "Rain mode engaged. Umbrellas are mandatory.",
  "Outside is wet. Inside is perfectly fine.",
  "Sky's washing the world. Walk slower.",
  "It is raining. Embrace the wet or escape it.",
  "Puddles are forming. Watch your step.",
  "Currently wet. The ducks are thrilled.",
  "Steady rain falling. Keep the hood up.",
  "Umbrella weather. Hold it tight.",
  "The sky is leaking. Try not to melt.",
  "Wet and wild out there. Stay as dry as you can."
};"""

new_snow = """static const char *const PHRASES_SNOW[] = {
  "Slush is the new ice. Step lighter.",
  "Cold and white out. Drive like grandma does.",
  "Roads strongly disagree with you today.",
  "Snow falling. Layer up and walk slow.",
  "Slippery world outside. Mind the curbs.",
  "Boots, gloves, patience. In that exact order.",
  "Hot drink mandatory. It's a snowy day rule.",
  "Sky is quiet, ground is loud. Wear layers.",
  "Snow stacking up. Boots are not optional.",
  "Winter wonderland vibes. But seriously, it's cold.",
  "Flakes are falling. Watch out for black ice.",
  "Snow mode. Everything takes ten minutes longer.",
  "Fresh powder. Great for photos, bad for driving.",
  "Bundle up fully. Winter is doing its thing.",
  "Snowy and slick. Stay upright if you can."
};"""

new_hot = """static const char *const PHRASES_HOT[] = {
  "Pavement is literally lava. Choose grass.",
  "AC is a love language. Stay intimately close to yours.",
  "Sweat is the bold new accessory. Drink water now.",
  "Hydrate or wilt. It is cooking out there.",
  "It is hot. The car will be significantly hotter.",
  "Heat advisory active. Vibes only inside today.",
  "Stay cool. Literally and figuratively.",
  "Heat wave conditions. Slow is fast today.",
  "Sun's not playing. Find the shade immediately.",
  "Baking temperature. Stay hydrated.",
  "It's a scorcher. Dress like you're on a beach.",
  "Oppressive heat. Don't push it outdoors.",
  "Roasting outside. The AC is your best friend.",
  "Hotter than a jalapeno's armpit. Drink up.",
  "Summer is aggressive today. Seek iced beverages."
};"""

new_cold = """static const char *const PHRASES_COLD[] = {
  "Winter said prove it. Wear heavy layers.",
  "Cold enough to taste. Hat now, regret later.",
  "Toe-numbing logic. Cover your extremities immediately.",
  "Layers are not a suggestion. Bundle up safely.",
  "It's cold. Your nose already knows the truth.",
  "Bone-cold outside. Hot bath waiting at home.",
  "Wind chill bites. Two pairs of socks is wisdom.",
  "Frost on everything. Plan extra driving time.",
  "Hands in pockets, head down. Move with intent.",
  "Frigid out. Thermal underwear is justified.",
  "Freezing conditions. Don't be a hero, wear the scarf.",
  "Arctic vibes today. Keep the core warm.",
  "Shiver weather. The heaviest coat is required.",
  "Ice box out there. Stay toasty.",
  "Seriously cold. Minimize outdoor exposure."
};"""

new_wind = """static const char *const PHRASES_WIND[] = {
  "Hair plans? Canceled. Lean heavily into the breeze.",
  "Trash bag flight risk. Secure your outdoor belongings.",
  "Gusts strong enough to rearrange your posture.",
  "Hold your hat. Wind is in charge today.",
  "Blustery conditions. Tie down loose stuff.",
  "Door-slam day. Hold the handle tightly.",
  "Wind makes mild cold feel completely arctic.",
  "Watch the trees. They'll tell on the wind.",
  "Crosswinds bite. Drive slower on highways.",
  "It's howling out there. Button up.",
  "High wind warning. Small pets might blow away.",
  "Gale force vibes. Lean into it.",
  "The wind is aggressive. Keep a low profile.",
  "Very breezy. Secure your hat and your dignity.",
  "Windy chaos. Umbrellas will invert instantly."
};"""

new_uv = """static const char *const PHRASES_HIGH_UV[] = {
  "The sun is heavily judging your sensitive skin.",
  "Shade is currency. Ensure you spend wisely.",
  "Burns happen in 15 minutes. Sunscreen isn't optional.",
  "UV's completely loud. Cover yourself up.",
  "Strong sun today. Strong shade preference required.",
  "Reapply your SPF. Then reapply it once again.",
  "Hat over hood. Wide brim always wins here.",
  "Sunglasses act as tools. They aren't just vibes.",
  "High UV. Bake in the oven, not outside.",
  "Radiation is high. Protect the epidermis.",
  "Scorching UV. Don't turn into a lobster.",
  "Sun is relentless. SPF is your shield.",
  "Maximum UV exposure. Cover up entirely.",
  "Solar rays are intense. Stay shaded.",
  "Skin damage risk high. Slather on the lotion."
};"""

new_air = """static const char *const PHRASES_BAD_AIR[] = {
  "Air feels terribly thick. Trust the AQI gauge.",
  "Lungs strongly prefer the indoors today.",
  "Indoor cardio officially wins. Keep windows shut tight.",
  "Air quality off. Mask up if sensitive.",
  "Outdoor workout gently postponed. Do not push it.",
  "Breathe shallow. Or just breathe safely inside.",
  "Air's having a rough one. Filter check time.",
  "Eyes burning is your body's early warning.",
  "Smoke or smog? Yes. Move your plans inside.",
  "AQI is sad. Stay in the filtered bubbles.",
  "Poor air quality. Not a day for deep breaths.",
  "Hazy conditions. Protect your respiratory system.",
  "Dirty air today. The purifier is working hard.",
  "Smoggy skies. Health trumps outdoor plans.",
  "Unhealthy air. Retreat to the indoors."
};"""

new_muggy = """static const char *const PHRASES_MUGGY[] = {
  "You're swimming. Embrace the completely dewy look.",
  "Cotton truly lies. You must wear synthetics today.",
  "Frizz is a total vibe. Hair plans firmly canceled.",
  "Sticky conditions. Shower-after-walking territory.",
  "Air is thick today. Move slow and hydrate.",
  "Humidity says hi. Very, very loudly.",
  "Towel in the bag. Always carry one today.",
  "Tropical conditions. Without the actual beach.",
  "Dew point over air temp. Welcome to soup.",
  "Muggy and gross. Stay near a fan.",
  "Like a sauna out there. Move sparingly.",
  "High humidity. You will sweat standing still.",
  "Swamp weather. Deodorant is working overtime.",
  "Sticky and humid. The AC is earning its keep.",
  "Air you can wear. Stay cool and dry."
};"""

new_pleasant = """static const char *const PHRASES_PLEASANT[] = {
  "Boring weather. This is officially the good kind.",
  "Goldilocks day. Make sure you use it.",
  "The exact weather your other apps brag about.",
  "Just nice out. Go safely enjoy something.",
  "A walk inside would be entirely wasted.",
  "All systems fully go. Step boldly outside.",
  "Friendly sky above. Go be friendly right back.",
  "Picnic-grade conditions. Grab your basket and run.",
  "No excuse weather. Go do the amazing thing.",
  "Perfect conditions. Don't waste it on a screen.",
  "Absolute perfection. Windows open, vibes high.",
  "Flawless day. Enjoy the great outdoors.",
  "Stellar weather. Go soak it up.",
  "Ideal temperature. Everyone's in a good mood.",
  "Simply delightful. Take the long way home."
};"""

replacements = {
    r'static const char \*const PHRASES_STORM\[\] = \{.*?\};': new_storms,
    r'static const char \*const PHRASES_RAIN_SOON\[\] = \{.*?\};': new_rain_soon,
    r'static const char \*const PHRASES_RAIN_NOW\[\] = \{.*?\};': new_rain_now,
    r'static const char \*const PHRASES_SNOW\[\] = \{.*?\};': new_snow,
    r'static const char \*const PHRASES_HOT\[\] = \{.*?\};': new_hot,
    r'static const char \*const PHRASES_COLD\[\] = \{.*?\};': new_cold,
    r'static const char \*const PHRASES_WIND\[\] = \{.*?\};': new_wind,
    r'static const char \*const PHRASES_HIGH_UV\[\] = \{.*?\};': new_uv,
    r'static const char \*const PHRASES_BAD_AIR\[\] = \{.*?\};': new_air,
    r'static const char \*const PHRASES_MUGGY\[\] = \{.*?\};': new_muggy,
    r'static const char \*const PHRASES_PLEASANT\[\] = \{.*?\};': new_pleasant,
}

for pat, repl in replacements.items():
    code = re.sub(pat, repl, code, flags=re.DOTALL)

with open('src/c/cards/advice.c', 'w') as f:
    f.write(code)

print("Phrases updated successfully.")
