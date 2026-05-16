import re

with open('phrases_review.md', 'r') as f:
    text = f.read()

tiers = [
    "STORM", "RAIN SOON", "RAIN NOW", "SNOW", "HOT", "COLD", "WIND", 
    "HIGH UV", "BAD AIR", "MUGGY", "PLEASANT"
]

var_names = [
    "PHRASES_STORM", "PHRASES_RAIN_SOON", "PHRASES_RAIN_NOW", "PHRASES_SNOW", 
    "PHRASES_HOT", "PHRASES_COLD", "PHRASES_WIND", "PHRASES_HIGH_UV", 
    "PHRASES_BAD_AIR", "PHRASES_MUGGY", "PHRASES_PLEASANT"
]

blocks = re.split(r'## \d+\. .*?\n', text)[1:]

replacements = {}
for i, block in enumerate(blocks):
    lines = block.strip().split('\n')
    phrases = []
    for line in lines:
        match = re.match(r'\d+\.\s+"(.*?)"', line)
        if match:
            phrases.append(f'  "{match.group(1)}"')
    
    var_name = var_names[i]
    c_array = f'static const char *const {var_name}[] = {{\n' + ',\n'.join(phrases) + '\n};'
    
    pattern = r'static const char \*const ' + var_name + r'\[\] = \{.*?\};'
    replacements[pattern] = c_array

with open('update_phrases.py', 'r') as f:
    update_script = f.read()

for pat, repl in replacements.items():
    update_script = re.sub(pat, repl, update_script, flags=re.DOTALL)

with open('update_phrases.py', 'w') as f:
    f.write(update_script)

