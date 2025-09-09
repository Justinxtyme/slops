#!/home/slapper/myenv/bin/python
from PIL import Image, ImageDraw, ImageFont
import freetype

# Parameters
FONT_PATH = "ssfiracode.ttf"  # Subsetted TTF font (U+0020–007E)
SAVE_FILE = "ssfiracode.png"
GLYPH_WIDTH = 8
GLYPH_HEIGHT = 16
COLUMNS = 12
ROWS = 8
START_CHAR = 0x20
END_CHAR = 0x7E

# Create blank grid image
grid_img = Image.new("1", (COLUMNS * GLYPH_WIDTH, ROWS * GLYPH_HEIGHT), 0)

# Load font
face = freetype.Face(FONT_PATH)
face.set_pixel_sizes(GLYPH_WIDTH, GLYPH_HEIGHT)

# Render each glyph
for i, codepoint in enumerate(range(START_CHAR, END_CHAR + 1)):
    row = i // COLUMNS
    col = i % COLUMNS
    x = col * GLYPH_WIDTH
    y = row * GLYPH_HEIGHT

    face.load_char(chr(codepoint), freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO)
    bitmap = face.glyph.bitmap

    # Create glyph image
    glyph_img = Image.new("1", (GLYPH_WIDTH, GLYPH_HEIGHT), 0)
    for py in range(bitmap.rows):
        for px in range(bitmap.width):
            byte = bitmap.buffer[py * bitmap.pitch + (px // 8)]
            if byte & (0x80 >> (px % 8)):
                glyph_img.putpixel((px, py), 1)

    grid_img.paste(glyph_img, (x, y))

# Save output
grid_img.save(SAVE_FILE)
print("Saved font grid as %s", SAVE_FILE)
