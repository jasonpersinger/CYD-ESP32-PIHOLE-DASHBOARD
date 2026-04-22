#!/usr/bin/env python3
"""Renders PNG preview images of each dashboard slide using sample data."""

from PIL import Image, ImageDraw, ImageFont
import os

W, H = 320, 240
SCALE = 2  # upscale for README readability

# RGB565 → RGB888
def c(rgb565):
    r = ((rgb565 >> 11) & 0x1F) * 255 // 31
    g = ((rgb565 >>  5) & 0x3F) * 255 // 63
    b = ( rgb565        & 0x1F) * 255 // 31
    return (r, g, b)

BG       = c(0x0820)
ACCENT   = c(0x07FF)
GREEN    = c(0x07E0)
RED      = c(0xF800)
ORANGE   = c(0xFD20)
WHITE    = c(0xFFFF)
GRAY     = c(0x8410)
DARKGRAY = c(0x2104)

# Sample data
DNS_QUERIES   = 15847
ADS_BLOCKED   = 3241
ADS_PCT       = 20.5
DOMAINS       = 1_127_453
CACHED        = 4892
CLIENTS       = 12
SLIDE_COUNT   = 3

def load_font(size):
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf",
    ]
    for path in candidates:
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()

F1 = load_font(10)
F2 = load_font(16)
F3 = load_font(22)
F4 = load_font(30)

def text_size(draw, text, font):
    bb = draw.textbbox((0, 0), text, font=font)
    return bb[2] - bb[0], bb[3] - bb[1]

def centered(draw, text, font, color, x, y):
    tw, th = text_size(draw, text, font)
    draw.text((x - tw // 2, y - th // 2), text, font=font, fill=color)

def left(draw, text, font, color, x, y):
    _, th = text_size(draw, text, font)
    draw.text((x, y - th // 2), text, font=font, fill=color)

def right(draw, text, font, color, x, y):
    tw, th = text_size(draw, text, font)
    draw.text((x - tw, y - th // 2), text, font=font, fill=color)

def draw_header(draw, title, color, slide_idx):
    draw.rectangle([0, 0, W, 36], fill=color)
    centered(draw, title, F2, BG, W // 2, 18)
    dot_x = (W // 2) - (SLIDE_COUNT * 14) // 2
    for i in range(SLIDE_COUNT):
        col = WHITE if i == slide_idx else DARKGRAY
        cx = dot_x + i * 14
        draw.ellipse([cx - 4, H - 12, cx + 4, H - 4], fill=col)

def big_stat(draw, label, value, value_color, y):
    centered(draw, label, F1, GRAY, W // 2, y)
    centered(draw, value, F3, value_color, W // 2, y + 22)

def bar_stat(draw, label, value, total, bar_color, y):
    left(draw,  label,       F1, GRAY,  10,     y)
    right(draw, str(value),  F1, WHITE, W - 10, y)
    bar_w = int((W - 20) * value / total) if total > 0 else 0
    draw.rectangle([10, y + 12, W - 10, y + 22], fill=DARKGRAY)
    if bar_w > 0:
        draw.rectangle([10, y + 12, 10 + bar_w, y + 22], fill=bar_color)

def slide_overview():
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    draw_header(d, "PI-HOLE DASHBOARD", GRAY, 0)
    pct_color = GREEN if ADS_PCT > 10 else ORANGE
    centered(d, f"{ADS_PCT:.1f}%", F4, pct_color, W // 2, 85)
    centered(d, "ADS BLOCKED TODAY", F1, GRAY, W // 2, 115)
    d.line([20, 130, W - 20, 130], fill=DARKGRAY)
    big_stat(d, "QUERIES TODAY", str(DNS_QUERIES), WHITE, 148)
    big_stat(d, "BLOCKED TODAY", str(ADS_BLOCKED), GREEN, 193)
    return img

def slide_blocking():
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    draw_header(d, "BLOCKING STATS", GRAY, 1)
    forwarded = DNS_QUERIES - ADS_BLOCKED - CACHED
    bar_stat(d, "QUERIES TODAY", DNS_QUERIES, DNS_QUERIES, ACCENT,  50)
    bar_stat(d, "BLOCKED",       ADS_BLOCKED, DNS_QUERIES, GREEN,   90)
    bar_stat(d, "CACHED",        CACHED,      DNS_QUERIES, ORANGE, 130)
    bar_stat(d, "FORWARDED",     forwarded,   DNS_QUERIES, GRAY,   170)
    return img

def slide_blocklist():
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    draw_header(d, "BLOCKLIST", GRAY, 2)
    big_stat(d, "DOMAINS BLOCKED", f"{DOMAINS:,}", RED,    80)
    d.line([20, 122, W - 20, 122], fill=DARKGRAY)
    big_stat(d, "CLIENTS SEEN",    str(CLIENTS),   ORANGE, 140)
    big_stat(d, "CACHED QUERIES",  str(CACHED),    ACCENT, 193)
    return img

out = os.path.join(os.path.dirname(__file__), "..", "docs")
os.makedirs(out, exist_ok=True)

for name, slide in [("slide1_overview", slide_overview()),
                    ("slide2_blocking", slide_blocking()),
                    ("slide3_blocklist", slide_blocklist())]:
    path = os.path.join(out, f"{name}.png")
    slide.resize((W * SCALE, H * SCALE), Image.NEAREST).save(path)
    print(f"Saved {path}")
