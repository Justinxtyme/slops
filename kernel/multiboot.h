#include <stdint.h>

struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
};




struct mb2_tag {
    uint32_t type;
    uint32_t size;
    // Payload follows depending on type
};

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
