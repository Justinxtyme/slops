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
#include "fat.h"



static inline void cpu_halt(void) {
    for(;;){ __asm__ __volatile__("cli; hlt"); }
}


static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
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

    fb_clear(0x00000000);
    fb_cursor_reset();
    ShellContext shell = { .running = 1 };
    init_shell_lines(&shell);
    kbd_init(); 
    init_kbd_state();
    draw_prompt();
    //fs_list_files();
    //print_file("HELLO2.TXT", &shell);
    
    for (;;) {
        __asm__ __volatile__("sti; hlt"); // enable interrupts, sleep until IRQ
        read_sc(&shell);                  // drain after wake
    }


    asm volatile("cli; hlt");

    outw(0x604, 0x2000); // QEMU exits with code 0

}


