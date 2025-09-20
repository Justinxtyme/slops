#ifndef FAT_H 
#define FAT_H

#include "types.h"
#include "shell.h"

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;
    uint16_t sectors_per_fat;
    uint32_t root_start_lba;
    uint32_t data_start_lba;
} fat_bpb;


enum FATType { 
    FAT12,
    FAT16, 
    FAT32 
};


typedef struct {
    enum FATType type;
    fat_bpb bpb;
    // maybe add root_cluster for FAT32
} fat_fs;


typedef struct {
    char name[9]; // null-terminated
    char ext[4];  // null-terminated
    uint8_t attr;
    uint16_t first_cluster;
    uint32_t size;
} fat_dir_entry;

int fs_parse_boot_sector(fat_bpb* bpb);

int parse_dir_entry(const uint8_t* e, fat_dir_entry* out);

int find_root_entry(const fat_bpb* bpb, const char* name83, fat_dir_entry* out);

uint16_t fat12_get_next_cluster(uint16_t cluster, const fat_bpb* bpb);

int fs_read_file(const char* name83, uint8_t* out, size_t maxlen);

int print_file(char *filename, ShellContext *shell);

#endif