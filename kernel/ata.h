#ifndef ATA_H 
#define ATA_H

#include <stdint.h>
#include <stddef.h>


void ata_read_sector(uint32_t lba, uint8_t* buffer);

int dump_mem(uint8_t* buffer, size_t length);

void cmd_dump_sector(int lba);

void read_root(void);

void read_sec(int lba);

#endif