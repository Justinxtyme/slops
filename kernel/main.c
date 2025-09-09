// kernel/main.c
#include <stdint.h>
#include <multiboot.h>
#include <gdt.h>
#include <idt.h>
#include <serial.h>
#include <font8x16.h>
#include <firacode.h>
#include <types.h>
#include <framebuffer.h>
#include <vga.h>


static inline void cpu_halt(void) {
    for(;;){ __asm__ __volatile__("cli; hlt"); }
}


///////////////////////////////////////////////////////////////
// quick test for proper GDT setup////////////////////////////
/////////////////////////////////////////////////////////////
static void log_gdt_state(void) {
    uint16_t cs, ds, es, ss;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile ("mov %%ds, %0" : "=r"(ds));
    __asm__ volatile ("mov %%es, %0" : "=r"(es));
    __asm__ volatile ("mov %%ss, %0" : "=r"(ss));

    serial_write("CS: "); serial_write_char('0' + ((cs >> 4) & 0xF)); serial_write_char('0' + (cs & 0xF)); serial_write("\n");
    serial_write("DS: "); serial_write_char('0' + ((ds >> 4) & 0xF)); serial_write_char('0' + (ds & 0xF)); serial_write("\n");
    serial_write("ES: "); serial_write_char('0' + ((es >> 4) & 0xF)); serial_write_char('0' + (es & 0xF)); serial_write("\n");
    serial_write("SS: "); serial_write_char('0' + ((ss >> 4) & 0xF)); serial_write_char('0' + (ss & 0xF)); serial_write("\n");

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) gdtr;

    __asm__ volatile ("sgdt %0" : "=m"(gdtr));

    serial_write("GDTR.limit: "); serial_write_char('0' + ((gdtr.limit >> 4) & 0xF)); serial_write_char('0' + (gdtr.limit & 0xF)); serial_write("\n");
    serial_write("GDTR.base: ");  // Just dump low byte for sanity
    serial_write_char('0' + ((gdtr.base >> 4) & 0xF)); serial_write_char('0' + (gdtr.base & 0xF)); serial_write("\n");
}



static void trigger_ud(void) {
    asm volatile ("ud2"); // guaranteed invalid opcode
}
 
static void trigger_pf(void) {
    volatile uint8_t *ptr = (uint8_t*)0x100000000ULL; // 4 GiB
    *ptr = 0xAA; // write to unmapped address
}

static void trigger_gp(void) {
    asm volatile ("movw %0, %%ds" :: "r"((uint16_t)0x23) : "memory");
}

void test_exceptions(void) {
    //sfprint("Triggering #UD...\n");
    //trigger_ud();

    sfprint("Triggering #PF...\n");
    trigger_pf();

    sfprint("Triggering #GP...\n");
    trigger_gp();
}

///////////////////////////////////////////////////////////////
//ENTRY POINT FROM BOOTLOAD///////////////////////////////////
/////////////////////////////////////////////////////////////
void kernel_main(void* mb_info) {
    outb(0xE9, 'M'); // debug marker
    gdt_init();
    gdt_install();
    set_all_idt();
    __asm__ volatile ("lidt %0" : : "m"(idtr));
    //vga_clear(ATTR);
    serial_init();
    serial_write("Hello from kernel_main!\n");
    log_gdt_state();
    //vga_clear(ATTR);
    walk_mb2(mb_info);
    //asm volatile ("int $8"); // vector 8, error code
    //asm volatile ("movw %0, %%ds" :: "r"((uint16_t)0x23) : "memory");
    //test_exceptions();
    //cpu_halt();
    fb_draw_string("Hello framebuffer.... and world!", 0x00FFFFFF, 0x00000000);
    fb_clear(0x00000000);
    fb_cursor_reset();
    
    while (1) {
        fb_draw_string(".", 0x00FFFFFF, 0x00000000);
        sfprint(".");
        //fb_clear(0x00000000);

        for (volatile int i = 0; i < 100000000; ++i); // crude delay
        fb_clear(0x00000000);
    }
}


