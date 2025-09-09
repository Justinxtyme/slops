from PIL import Image

def convert_font(image_path, char_width, char_height, num_chars, output_file):
    """
    Converts a bitmap font sheet image into a C header file array.
    
    Args:
        image_path (str): Path to the font sheet image (e.g., 'font.png').
        char_width (int): The width of a single character in pixels.
        char_height (int): The height of a single character in pixels.
        num_chars (int): The number of characters in the font sheet.
        output_file (str): The path to the output C header file.
    """
    try:
        image = Image.open(image_path).convert('1')  # Convert to 1-bit monochrome
    except FileNotFoundError:
        print(f"Error: Image file not found at {image_path}")
        return

    img_width, img_height = image.size
    
    if img_width % char_width != 0 or img_height % char_height != 0:
        print("Error: Image dimensions are not a multiple of character dimensions.")
        return

    # Calculate number of characters per row in the image
    chars_per_row = img_width // char_width
    
    with open(output_file, 'w') as f:
        f.write("/* Generated font data from " + image_path + " */\n\n")
        f.write(f"const unsigned char font_data[{num_chars}][{char_height}] = {{\n")

        for char_index in range(num_chars):
            # Calculate character's position in the font sheet
            x_offset = (char_index % chars_per_row) * char_width
            y_offset = (char_index // chars_per_row) * char_height
            
            # Extract the character's bitmap
            char_image = image.crop((x_offset, y_offset, x_offset + char_width, y_offset + char_height))

            # Convert to C array format
            f.write("        { ")
            for y in range(char_height):
                byte_value = 0
                for x in range(char_width):
                    if char_image.getpixel((x, y)) != 0: # Black pixel
                        # Set the corresponding bit in the byte
                        byte_value |= (1 << (char_width - 1 - x))
                f.write(f"0x{byte_value:02X}, ")
            f.write(" },\n")

        f.write("};\n")
        print(f"Conversion complete! Output saved to {output_file}")

# --- Configuration ---
# Example for a font sheet with 16 characters per row, 8x16 pixels each.
# Make sure your font sheet image is correctly laid out.
font_sheet_image = 'my_font_sheet.png'
character_width = 8
character_height = 16
total_characters = 256 # For standard ASCII
output_c_header = 'my_font.h'

convert_font('ssfiracode.png', 8, 16, 256, 'firacode.h')
