#!/usr/bin/env python3
"""Generate the TouchyWeather menu icon (25x25 PNG).

Design: bold black silhouette of sun + cloud on transparent background.
At 25x25 + auto-greyscale conversion in the Pebble launcher, a strong
two-tone silhouette reads far better than a colored, outlined illustration.

Renders at 32x then downsamples with Lanczos for crisp anti-aliased edges.
"""
import math
import os
from PIL import Image, ImageDraw

SIZE = 25
SCALE = 32
W = SIZE * SCALE

img = Image.new("RGBA", (W, W), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

INK = (0, 0, 0, 255)
CLEAR = (0, 0, 0, 0)

# --- Sun (upper-right) ---
sun_cx, sun_cy = int(W * 0.66), int(W * 0.32)
sun_r = int(W * 0.18)

# 8 chunky rays as thick lines with round caps
ray_inner = sun_r + int(W * 0.04)
ray_outer = sun_r + int(W * 0.14)
ray_half = int(W * 0.035)
for i in range(8):
    a = i * (math.pi / 4)
    x1 = sun_cx + ray_inner * math.cos(a)
    y1 = sun_cy + ray_inner * math.sin(a)
    x2 = sun_cx + ray_outer * math.cos(a)
    y2 = sun_cy + ray_outer * math.sin(a)
    d.line([(x1, y1), (x2, y2)], fill=INK, width=ray_half * 2)
    d.ellipse([x2 - ray_half, y2 - ray_half,
               x2 + ray_half, y2 + ray_half], fill=INK)

# Sun body — solid disc
d.ellipse([sun_cx - sun_r, sun_cy - sun_r,
           sun_cx + sun_r, sun_cy + sun_r], fill=INK)

# --- Cloud (lower-left, overlapping sun) ---
cb_y = int(W * 0.78)
c_left = int(W * 0.06)
c_right = int(W * 0.82)

puffs = [
    (int(W * 0.22), int(W * 0.62), int(W * 0.20)),
    (int(W * 0.46), int(W * 0.50), int(W * 0.26)),
    (int(W * 0.70), int(W * 0.60), int(W * 0.22)),
]
for cx, cy, r in puffs:
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=INK)
d.rectangle([c_left, cb_y - int(W * 0.14), c_right, cb_y], fill=INK)

# Round the cloud base corners
corner_r = int(W * 0.07)
d.ellipse([c_left - corner_r, cb_y - corner_r * 2,
           c_left + corner_r, cb_y], fill=INK)
d.ellipse([c_right - corner_r, cb_y - corner_r * 2,
           c_right + corner_r, cb_y], fill=INK)

# Downsample
out = img.resize((SIZE, SIZE), Image.LANCZOS)

dst = os.path.abspath(os.path.join(
    os.path.dirname(__file__), "..", "resources", "images", "menu_icon.png"))
out.save(dst, optimize=True)
print("wrote", dst, out.size)
