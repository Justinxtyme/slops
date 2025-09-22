// kassert.c
#include "assertf.h"
#include "serial.h"   // or fb_draw_string

void assert_fail(const char *expr, const char *file, int line) {
    sfprint("ASSERTION FAILED: %s\nFile: %s\nLine: %d\n", expr, file, line);
    __asm__ volatile("cli; hlt");
    for (;;) {} // just in case
}