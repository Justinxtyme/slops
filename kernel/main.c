// kernel/main.c
#include <stdint.h>
#include "multiboot.h"
#include "gdt.h"
#include "idt.h"
#include "serial.h"
#include "font8x16.h"
#include "firacode.h"
#include "types.h"
#include "framebuffer.h"
#include "vga.h"
#include "kbd.h"
#include "mem.h"
#include "shell.h"
#include "ata.h"



static inline void cpu_halt(void) {
    for(;;){ __asm__ __volatile__("cli; hlt"); }
}



///////////////////////////////////////////////////////////////
//ENTRY POINT FROM BOOTLOAD///////////////////////////////////
/////////////////////////////////////////////////////////////
void kernel_main(void* mb_info) {
    //outb(0xE9, 'M'); // debug marker
    uint8_t status = inb(0x60);
    sfprint("Initial ATA status: %h\n", status);
//initialize serial output
    serial_init();
    serial_write("Hello from kernel_main!\n");

    // initialize GDT and IDT and MEM
    gdt_init();
    gdt_install();
    set_all_idt();
    __asm__ volatile ("lidt %0" : : "m"(idtr));
    remap_pic();
    enable_irq();
    asm volatile("sti");
    log_gdt_state();
    mem_init();
    
    // Walk multiboot header to pull necessary data and  
    walk_mb2(mb_info);

    init_shell_lines();
    //fb_draw_string("Hello framebuffer.... and world!", 0x00FFFFFF, 0x00000000);
    //for (volatile int i = 0; i < 1000000000; ++i); // crude delay
    fb_clear(0x00000000);
    fb_cursor_reset();
    ShellContext shell = { .running = 1 };
    int *arr = thralloc(1024);
    thralloc_total();
    tfree(arr);
    thralloc_total();
    cmd_dump_sector(0);
    kbd_init(); 
    init_kbd_state();
    draw_prompt();
    
    while (shell.running) {
        asm volatile("cli");
        //fb_draw_string("J", 0x00FFFFFF, 0x00000000);
        //sfprint(".");
        //fb_clear(0x00000000);
        read_sc(&shell);
        //for (volatile int i = 0; i < 100000000; ++i); // crude delay
        //fb_clear(0x00000000);
        asm volatile("sti");
    }
}


