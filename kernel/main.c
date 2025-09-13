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



static inline void cpu_halt(void) {
    for(;;){ __asm__ __volatile__("cli; hlt"); }
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
    remap_pic();
    enable_irq();
    asm volatile("sti");
    //vga_clear(ATTR);
    serial_init();
    serial_write("Hello from kernel_main!\n");
    log_gdt_state();
    //vga_clear(ATTR);
    walk_mb2(mb_info);
    //asm volatile ("movw %0, %%ds" :: "r"((uint16_t)0x23) : "memory");
    //test_exceptions();
    init_shell_lines();
    fb_draw_string("Hello framebuffer.... and world!", 0x00FFFFFF, 0x00000000);
    for (volatile int i = 0; i < 1000000000; ++i); // crude delay
    fb_clear(0x00000000);
    fb_cursor_reset();
    ShellContext shell = { .running = 1 };
    mem_init();
    int *arr = thralloc(1000);
    thralloc_total();
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


