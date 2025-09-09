//multiboot.h
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
};




struct mb2_tag {
    uint32_t type;
    uint32_t size;
    // Payload follows depending on type
} __attribute__((packed));

struct multiboot_tag_string {
    uint32_t type;
    uint32_t size;
    char string[];
};

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    //struct multiboot_mmap_entry entries[];
};

struct mb2_tag_framebuffer {
    struct mb2_tag tag;          // type = 8, size = 32
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint8_t  reserved[2];

} __attribute__((packed));



// Define the struct for direct RGB color info (framebuffer_type == 1)
struct mb2_color_info_rgb {
    uint8_t framebuffer_red_field_position;
    uint8_t framebuffer_red_mask_size;
    uint8_t framebuffer_green_field_position;
    uint8_t framebuffer_green_mask_size;
    uint8_t framebuffer_blue_field_position;
    uint8_t framebuffer_blue_mask_size;
};

// Define the struct for palette color info (framebuffer_type == 0)
// The palette is a variable-length array.
struct mb2_color_info_palette {
    uint16_t framebuffer_palette_num_colors;
    // The palette data itself would follow here.
    // Use a Flexible Array Member (FAM) if using C99 or later.
    // struct { uint8_t red, green, blue; } palette_data[];
};

#endif