#include <stdint.h>
#include "fat.h"
#include "serial.h"
#include "types.h"
#include "ata.h"
#include "string.h"
#include "framebuffer.h"
#include "shell.h"



int fs_parse_boot_sector(fat_bpb* bpb) {
    uint8_t buf[512];  
    // Temporary buffer to hold the raw boot sector (sector 0) from disk
    ata_read_sector(0, buf);  
    // Read LBA 0 (boot sector) into buf — this contains the BPB (BIOS Parameter Block)
    // and possibly boot code. All FAT layout info comes from here.
    // --- Parse core BPB fields from fixed offsets in the boot sector ---
    bpb->bytes_per_sector    = buf[11] | (buf[12] << 8);  
    // Bytes per sector (u16) — usually 512, but read from BPB offset 0x0B
    bpb->sectors_per_cluster = buf[13];  
    // Sectors per cluster (u8) — cluster size in sectors, from BPB offset 0x0D
    bpb->reserved_sectors    = buf[14] | (buf[15] << 8);  
    // Reserved sectors before the first FAT — usually 1 (the boot sector itself), from offset 0x0E
    bpb->num_fats            = buf[16];  
    // Number of FAT copies — usually 2, from offset 0x10
    bpb->root_entry_count    = buf[17] | (buf[18] << 8);  
    // Max number of root directory entries (FAT12/16 only), from offset 0x11
    bpb->sectors_per_fat     = buf[22] | (buf[23] << 8);  
    // Sectors per FAT table (u16) — size of each FAT copy, from offset 0x16
    // --- Calculate derived layout values ---
    uint32_t root_dir_sectors = ((bpb->root_entry_count * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;
    // Root directory size in sectors:
    // Each root entry is 32 bytes; multiply by entry count to get total bytes,
    // then divide by bytes/sector (rounding up) to get sector count.
    bpb->root_start_lba = bpb->reserved_sectors + (bpb->num_fats * bpb->sectors_per_fat);
    // LBA where the root directory starts:
    // Skip reserved sectors + all FAT copies.
    bpb->data_start_lba = bpb->root_start_lba + root_dir_sectors;
    // LBA where the data region (cluster #2) starts:
    // Skip root directory sectors after the FATs.
    // sfprint("BPS: %8\nSPC: %8\nRES: %8\nFATS: %8\nREC: %8\nSPF: %8\nRDS: %8\nRoot Start: %8\nData Start: %8\n", 
    //        bpb->bytes_per_sector, bpb->sectors_per_cluster, bpb->reserved_sectors, bpb->num_fats, bpb->root_entry_count,
    //        bpb->sectors_per_fat, root_dir_sectors, bpb->root_start_lba, bpb->data_start_lba);
    
    return 1;   
    // Return struct success — at this point, bpb contains enough info to locate any file/dir.
}


int parse_dir_entry(const uint8_t* e, fat_dir_entry* out) {
    // Skip entries that are not valid files/directories:
    // e[0]:q == 0x00 → no more entries in this directory (end marker)
    // e[11] == 0x0F → long file name (LFN) entry, not a short 8.3 entry
    // e[0] == 0xE5 → deleted entry
    if (e[0] == 0x00 || e[11] == 0x0F || e[0] == 0xE5) return 0;
    // Extract the 8-character name field (offset 0x00–0x07)
    // Replace space padding with '\0' to terminate the string early
    for (int i = 0; i < 8; i++) {
        out->name[i] = (e[i] == ' ') ? '\0' : e[i];
        sfprint("e[%d] = %c  i[%d] = %c\n", i, e[i], i, out->name[i]);
    }
    out->name[8] = '\0'; // Ensure null-termination
    // Extract the 3-character extension field (offset 0x08–0x0A)
    // Replace space padding with '\0'
    for (int i = 0; i < 3; i++) {
        out->ext[i] = (e[8 + i] == ' ') ? '\0' : e[8 + i];
    }
    out->ext[3] = '\0'; // Null-terminate
    // File attributes byte (offset 0x0B)
    // Bitfield: ReadOnly, Hidden, System, VolumeID, Directory, Archive
    out->attr = e[11];
    // First cluster number (offset 0x1A–0x1B)
    // FAT12/16: this is the full cluster number
    // FAT32: this is the low word; high word is at offset 0x14–0x15
    out->first_cluster = e[26] | (e[27] << 8);
    // File size in bytes (offset 0x1C–0x1F), little-endian
    out->size = e[28] | (e[29] << 8) | (e[30] << 16) | (e[31] << 24);
    // Return 1 to indicate a valid short directory entry was parsed
    char full[13]; // 8 chars for name + 1 dot + 3 chars for ext + null terminator
    int name_len = custom_strlen(out->name);
    int ext_len = custom_strlen(out->ext);
    for (int i = 0; i < name_len; i++) {
                full[i] = out->name[i];
    }
    if (ext_len > 0) {
        full[name_len] = '.';
        for (int i = 0; i < 3; i++) {
            full[i + (name_len +1)] = out->ext[i];
        }
        full[(name_len + 1) + 3] = '\0';
    } else {
        full[name_len] = '\0';
    }
    sfprint("Filename: %s\n", full);
    return 1;
}


int list_dir_entry(const uint8_t* e, fat_dir_entry* out, ShellContext *shell) {
    // Skip entries that are not valid files/directories:
    // e[0]:q == 0x00 → no more entries in this directory (end marker)
    // e[11] == 0x0F → long file name (LFN) entry, not a short 8.3 entry
    // e[0] == 0xE5 → deleted entry
    if (e[0] == 0x00 || e[11] == 0x0F || e[0] == 0xE5) {
        sfprint("skipped entry\n");
        return 0;
    }
    // Extract the 8-character name field (offset 0x00–0x07)
    // Replace space padding with '\0' to terminate the string early
    for (int i = 0; i < 8; i++) {
        out->name[i] = (e[i] == ' ') ? '\0' : e[i];
        //sfprint("e[%d] = %c  i[%d] = %c\n", i, e[i], i, out->name[i]);
    }
    out->name[8] = '\0'; // Ensure null-termination
    // Extract the 3-character extension field (offset 0x08–0x0A)
    // Replace space padding with '\0'
    for (int i = 0; i < 3; i++) {
        out->ext[i] = (e[8 + i] == ' ') ? '\0' : e[8 + i];
    }
    out->ext[3] = '\0'; // Null-terminate
    // File attributes byte (offset 0x0B)
    // Bitfield: ReadOnly, Hidden, System, VolumeID, Directory, Archive
    out->attr = e[11];
    // First cluster number (offset 0x1A–0x1B)
    // FAT12/16: this is the full cluster number
    // FAT32: this is the low word; high word is at offset 0x14–0x15
    out->first_cluster = e[26] | (e[27] << 8);
    // File size in bytes (offset 0x1C–0x1F), little-endian
    out->size = e[28] | (e[29] << 8) | (e[30] << 16) | (e[31] << 24);
    // Return 1 to indicate a valid short directory entry was parsed
    char full[13]; // 8 chars for name + 1 dot + 3 chars for ext + null terminator
    int name_len = custom_strlen(out->name);
    int ext_len = custom_strlen(out->ext);
    for (int i = 0; i < name_len; i++) {
        full[i] = out->name[i];
    }
    if (ext_len > 0) {
        full[name_len] = '.';
        for (int i = 0; i < 3; i++) {
            full[i + (name_len +1)] = out->ext[i];
        }
        full[(name_len + 1) + 3] = '\0';
    } else {
        full[name_len] = '\0';
    }
    sfprint("%s\n", full);
    fb_draw_string(full, FG, BG);
    fb_draw_string("\n", FG, BG);
    shell->shell_line++;
    return 1;
}

int list_root(const fat_bpb* bpb, fat_dir_entry* out, ShellContext *shell) {
    
    uint32_t root_dir_sectors =
        ((bpb->root_entry_count * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;
    uint8_t sector[512]; // Temporary buffer for one sector of directory entries

    // Loop through each sector of the root directory
    for (uint32_t s = 0; s < root_dir_sectors; s++) {
        //sfprint("loop %8\n", s);
        // Read the current root directory sector from disk
        ata_read_sector(bpb->root_start_lba + s, sector);
        sfprint("Sector read: %8\n", bpb->root_start_lba + s);

        for (int i = 0; i < 512; i += 32) {
            if (!list_dir_entry(sector + i, out, shell)) {
                continue;
            }  
        } 
    } 
    // File not found in the root directory
    sfprint("files printed\n");
    return 0;
}

int find_root_entry(const fat_bpb* bpb, const char* name83, fat_dir_entry* out) {
    uint32_t root_dir_sectors =
        ((bpb->root_entry_count * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;
    uint8_t sector[512]; // Temporary buffer for one sector of directory entries

    // Loop through each sector of the root directory
    for (uint32_t s = 0; s < root_dir_sectors; s++) {
        //sfprint("loop %8\n", s);
        // Read the current root directory sector from disk
        ata_read_sector(bpb->root_start_lba + s, sector);
        sfprint("Sector read: %8\n", bpb->root_start_lba + s);

        for (int i = 0; i < 512; i += 32) {
            if (!parse_dir_entry(sector + i, out)) {
                continue;
            }    
            char full[13]; // 8 chars for name + 1 dot + 3 chars for ext + null terminator
            int name_len = custom_strlen(out->name);
            int ext_len = custom_strlen(out->ext);
            for (int i = 0; i < name_len; i++) {
                full[i] = out->name[i];
            }
            if (ext_len > 0) {
                full[name_len] = '.';
                for (int i = 0; i < 3; i++) {
                    full[i + (name_len +1)] = out->ext[i];
                }
                full[(name_len + 1) + 3] = '\0';
            } else {
                full[name_len] = '\0';
            }
            //sfprint("FULL: %s\n", full);
            sfprint("%s.%s\n", out->name, out->ext);
            if (cst_strcmp(full, name83) == 1)
                return 1; // Found the file — 'out' now contains its metadata
        }
    }
    // File not found in the root directory
    sfprint("file '%s' not found\n", name83);
    return 0;
}

uint16_t fat12_get_next_cluster(uint16_t cluster, const fat_bpb* bpb) {
    // --- Calculate where in the FAT this cluster's entry lives ---
    // Each FAT12 entry is 12 bits (1.5 bytes), so to find the byte offset:
    // multiply the cluster number by 1.5 → (cluster * 3) / 2
    uint32_t fat_offset = (cluster * 3) / 2;
    // Convert that byte offset into an absolute sector number within the FAT area:
    // Start at the first FAT sector (after reserved sectors), then add the sector offset
    uint32_t fat_sector = bpb->reserved_sectors + (fat_offset / 512);
    // Offset within that sector where the 12-bit entry starts
    uint32_t ent_offset = fat_offset % 512;
    // --- Read the FAT sector containing this entry ---
    uint8_t sec[512];
    ata_read_sector(fat_sector, sec);
    // --- Extract the 12-bit value ---
    uint16_t next;
    if (cluster & 1) {
        // Odd cluster: the 12 bits start in the high nibble of the first byte
        // and continue into the next byte
        //
        // Layout for odd cluster N:
        //   byte0: xxxxHHHH  (low nibble belongs to previous cluster)
        //   byte1: HHHHHHHH  (high byte)
        //
        // Shift right to drop the previous cluster's bits, then combine
        next = ((sec[ent_offset] >> 4) | (sec[ent_offset + 1] << 4)) & 0x0FFF;
    } else {
        // Even cluster: the 12 bits start at the low nibble of the first byte
        // and continue into the low nibble of the next byte
        //
        // Layout for even cluster N:
        //   byte0: LLLLLLLL  (low byte)
        //   byte1: xxxxLLLL  (high nibble)
        //
        // Mask off the high nibble of byte1 to keep only our cluster's bits
        next = (sec[ent_offset] | ((sec[ent_offset + 1] & 0x0F) << 8)) & 0x0FFF;
    }
    // Return the next cluster number in the chain (0xFF8+ means end-of-chain)
    return next;
}

int fs_read_file(const char* name83, uint8_t* out, size_t maxlen) {
    fat_bpb bpb;
    fs_parse_boot_sector(&bpb);
    // Parse the boot sector into a BPB struct so we know the filesystem layout:
    // bytes/sector, sectors/cluster, reserved sectors, FAT size, root/data LBAs, etc.
    fat_dir_entry ent;
    if (!find_root_entry(&bpb, name83, &ent)) return -1;
    // Search the root directory for the given 8.3 filename.
    // If found, 'ent' will contain its starting cluster and file size.
    // If not found, return -1 to indicate error.
    uint16_t cluster = ent.first_cluster;
    // The first cluster number where this file's data begins.
    size_t remaining = ent.size;
    // Number of bytes left to read from the file.
    size_t written = 0;
    // Number of bytes successfully copied into 'out'.
    uint8_t sec[512];
    // Temporary buffer for reading one sector from disk.
    // Walk the FAT chain until we hit an end-of-chain marker or run out of bytes to read.
    while (cluster < 0xFF8 && remaining > 0) {
        // Convert cluster number to absolute LBA:
        // Data region starts at data_start_lba, and cluster #2 is the first data cluster.
        uint32_t lba = bpb.data_start_lba + (cluster - 2) * bpb.sectors_per_cluster;
        // Read all sectors in this cluster (usually 1 for floppies, but BPB defines it)
        for (uint8_t s = 0; s < bpb.sectors_per_cluster && remaining > 0; s++) {
            ata_read_sector(lba + s, sec);
            // Read one sector from disk into 'sec'.
            size_t chunk = (remaining < 512) ? remaining : 512;
            // Only copy as many bytes as remain in the file (last sector may be partial).
            for (size_t i = 0; i < chunk && written < maxlen; i++)
                out[written++] = sec[i];
            // Copy from sector buffer into output buffer.
            remaining -= chunk;
            // Reduce remaining byte count.
        }
        // Look up the next cluster in the FAT table.
        // For FAT12, this involves unpacking a 12-bit entry.
        cluster = fat12_get_next_cluster(cluster, &bpb);
    }
    // Return the number of bytes actually written to 'out'.
    return (int)written;
}

int fs_list_files(ShellContext *shell) {
    sfprint("\n\nListing files\n");
    fat_bpb bpb;
    fs_parse_boot_sector(&bpb);
    // Parse the boot sector into a BPB struct so we know the filesystem layout:
    // bytes/sector, sectors/cluster, reserved sectors, FAT size, root/data LBAs, etc.
    fat_dir_entry ent;
    list_root(&bpb, &ent, shell);
    return 0;
}

int print_file(char *filename, ShellContext *shell) {
    uint8_t buffer[4096];
    int len = fs_read_file(filename, buffer, sizeof(buffer));
    if (len > 0) {
        buffer[len] = '\0'; // if it's text
        sfprint("\n%s\n%s\n", filename, buffer);
        fb_draw_string("\n", 0x00FFFFFF, 0x00000000);
        shell->shell_line++;
        fb_draw_stringsh(buffer, 0x00FFFFFF, 0x00000000, shell);
        for (int i = 0; i < len; i++) {
            if (buffer[i] == '\n') {
                shell->shell_line++;
            }
        }
        sfprint("shell line: %d", shell->shell_line);
    } else {
        sfprint("No data detected in %c", filename);
    }
    return 0;
}