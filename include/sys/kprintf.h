#ifndef __KPRINTF_H
#define __KPRINTF_H

#include <sys/defs.h>

// not reentry safe
void kprintf(const char *fmt, ...);
// not reentry safe (extremely)
void kprintfa(char* dest, int size, const char *format, ...);

void clear_screen();

void print_topright(const char* str, uint8_t color);

void print_terminal_input_line(char* src, int length);

#endif
