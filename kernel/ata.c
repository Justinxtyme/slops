#include "ata.h" 
#include "serial.h"


// #define ATA_PRIMARY_IO     0x1F0 // base IO port for ATA
// #define ATA_PRIMARY_CTRL   0x3F6 // control port
// #define ATA_REG_DATA       0x1F0 // Data reg
// #define ATA_REG_ERROR      0x1F1 // error register or features(if write)
// #define ATA_REG_SECCOUNT0  0x1F2 // sector count
// #define ATA_REG_LBA0       0x1F3 // lowest byte of sector addy
// #define ATA_REG_LBA1       0x1F4 // next byte
// #define ATA_REG_LBA2       0x1F5 // next byte
// #define ATA_REG_HDDEVSEL   0x1F6 // drive/head selector , LBA flag mode
// #define ATA_REG_COMMAND    0x1F7 // command register
// #define ATA_REG_STATUS     0x1F7 // status register

// #define ATA_CMD_READ_PIO   0x20 
// #define ATA_STATUS_BSY     0x80
// #define ATA_STATUS_DRQ     0x08

// static inline uint16_t inw(uint16_t port) {
//     uint16_t result;
//     __asm__ volatile ("inw %1, %0" : "=a"(result) : "dN"(port));
//     return result;
// }


// void ata_wait() {
//     while (inb(ATA_REG_STATUS) & ATA_STATUS_BSY); // wait until not busy
//     while (!(inb(ATA_REG_STATUS) & ATA_STATUS_DRQ)); // wait until data ready
// }

// void ata_read_sector(uint32_t lba, uint8_t* buffer) {


//     uint8_t status = inb(ATA_REG_STATUS);
//     if (status == 0xFF || status == 0x00) {
//         sfprint("Status: %h No ATA device detected\n", status);
//         return;
//     }

//     sfprint("ata_read_sector: Select drive\n");
//     outb(ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F)); // select drive + LBA mode
//     sfprint("ata_read_sector: Read 1 sector\n");
//     outb(ATA_PRIMARY_CTRL, 0x04); // software reset
//     for (int i = 0; i < 100000; i++) {
//         if (!(inb(ATA_REG_STATUS) & ATA_STATUS_BSY)) break;
//     }
//     outb(ATA_REG_SECCOUNT0, 1);                          // read 1 sector
//     sfprint("ata_read_sector: Bits 0-7\n");
//     outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));           // LBA bits 0–7
//     sfprint("ata_read_sector: Bits 8-15\n");
//     outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));    // LBA bits 8–15
//     sfprint("ata_read_sector: Bits 16-23\n");
//     outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));   // LBA bits 16–23
//     sfprint("ata_read_sector: Issuing read command\n");
//     outb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);            // issue read command
//     sfprint("ata_read_sector: Waiting\n");
//     ata_wait(); // wait for drive to be ready

//     for (int i = 0; i < 256; i++) {                      // 256 words = 512 bytes
//         uint16_t data = inw(ATA_REG_DATA);               // read word from data port
//         buffer[i * 2]     = (uint8_t)(data & 0xFF);      // low byte
//         buffer[i * 2 + 1] = (uint8_t)(data >> 8);        // high byte
//     }
// }

// void dump_mem(uint8_t* buffer, size_t length) {
//     for (size_t i = 0; i < length; i++) {
//         if (i % 16 == 0) sfprint("\n%8: ", i); // print offset every 16 bytes
//         sfprint("%h ", buffer[i]);             // print byte in hex
//     }
//     sfprint("\n");
// }

// void cmd_dump_sector(int lba) {
//     uint8_t buffer[512];
//     ata_read_sector(lba, buffer);
//     dump_mem(buffer, 512);
// }


#define ATA_PRIMARY_IO     0x1F0
#define ATA_PRIMARY_CTRL   0x3F6

#define ATA_REG_DATA       0x1F0
#define ATA_REG_ERROR      0x1F1
#define ATA_REG_FEATURES   0x1F1
#define ATA_REG_SECCOUNT0  0x1F2
#define ATA_REG_LBA0       0x1F3
#define ATA_REG_LBA1       0x1F4
#define ATA_REG_LBA2       0x1F5
#define ATA_REG_HDDEVSEL   0x1F6
#define ATA_REG_COMMAND    0x1F7
#define ATA_REG_STATUS     0x1F7

#define ATA_CMD_READ_PIO   0x20

#define ATA_STATUS_ERR     0x01
#define ATA_STATUS_DRQ     0x08
#define ATA_STATUS_SRV     0x10
#define ATA_STATUS_DF      0x20
#define ATA_STATUS_DRDY    0x40
#define ATA_STATUS_BSY     0x80


static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "dN"(port));
    return result;
}


static inline void ata_400ns_delay(void) {
    (void)inb(ATA_REG_STATUS);
    (void)inb(ATA_REG_STATUS);
    (void)inb(ATA_REG_STATUS);
    (void)inb(ATA_REG_STATUS);
}

static int ata_wait_busy_clear(uint32_t spins) {
    while (spins--) {
        uint8_t s = inb(ATA_REG_STATUS);
        if (!(s & ATA_STATUS_BSY)) return 1;
    }
    return 0;
}

static int ata_wait_drq(uint32_t spins) {
    while (spins--) {
        uint8_t s = inb(ATA_REG_STATUS);
        if (s & (ATA_STATUS_ERR | ATA_STATUS_DF)) return -1;
        if ((s & (ATA_STATUS_BSY | ATA_STATUS_DRQ)) == ATA_STATUS_DRQ) return 1;
    }
    return 0;
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    // Optional: disable IRQs from drive (nIEN=1 at ctrl), but DO NOT assert SRST
    outb(ATA_PRIMARY_CTRL, 0x02); // nIEN=1, SRST=0

    // Select drive (master) + LBA high nybble
    outb(ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    ata_400ns_delay();

    // Program sector count and 28-bit LBA
    outb(ATA_REG_SECCOUNT0, 1);
    outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));

    // Issue READ SECTORS
    outb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    // Wait for BSY=0 then DRQ=1
    if (!ata_wait_busy_clear(1000000)) {
        sfprint("ATA: timeout waiting BSY clear\n");
        return;
    }
    int drq = ata_wait_drq(1000000);
    if (drq <= 0) {
        if (drq < 0) {
            uint8_t err = inb(ATA_REG_ERROR);
            sfprint("ATA: ERR/DF during read, ERR=%h\n", err);
        } else {
            sfprint("ATA: timeout waiting DRQ\n");
        }
        return;
    }

    // Read 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
        uint16_t w = inw(ATA_REG_DATA);
        buffer[i*2+0] = (uint8_t)(w & 0xFF);
        buffer[i*2+1] = (uint8_t)(w >> 8);
    }
}


int dump_mem(uint8_t* buffer, size_t length) {
    size_t i;
    for (i = 0; i < length; i++) {
        if (i % 16 == 0) sfprint("\n%8: ", i); // print offset every 16 bytes
        sfprint("%h ", buffer[i]);             // print byte in hex
    }
    sfprint("\n");
    sfprint("i = %8\n", buffer[length - 1]);
    if (buffer[length - 1] == 0) {
        return 0;
    }
    return 1;
}

void cmd_dump_sector(int lba) {
    uint8_t buffer[512];
    ata_read_sector(lba, buffer);
    int more = dump_mem(buffer, 512); 
    sfprint("more: %d\n", more);
    if (more) {
        cmd_dump_sector(lba + 1);
        return;
    }
}