import re

with open('src/c/cards/advice.c', 'r') as f:
    code = f.read()

# Fix unused function warning and ensure prv_generate_tier_headline is correctly placed
# It was placed inside, but wait, the compiler said "defined but not used" and there's a missing semicolon in my python replacement.

code = code.replace("prv_generate_tier_headline(tier, d, headline_text, sizeof(headline_text));", "prv_generate_tier_headline(tier, d, headline_text, sizeof(headline_text));")

with open('src/c/cards/advice.c', 'w') as f:
    f.write(code)

