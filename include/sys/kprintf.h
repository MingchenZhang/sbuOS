#ifndef __KPRINTF_H
#define __KPRINTF_H

// not reentry safe
void kprintf(const char *fmt, ...);
// not reentry safe (extremely)
void kprintfa(char* dest, int size, const char *format, ...);

void clear_screen();

void print_topright(const char* str, uint8_t color);

#endif
