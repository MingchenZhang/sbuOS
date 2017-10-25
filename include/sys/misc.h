#include <sys/defs.h>

#ifndef _MISC_H
#define _MISC_H

extern volatile uint64_t pic_tick_count;
extern uint8_t last_key_pressed[];

void asm_outb(uint16_t port, uint8_t value);
uint8_t asm_inb(uint16_t port);

void asm_outl(uint16_t port, uint32_t value);
uint32_t asm_inl(uint16_t port);

void update_topright_display();
uint32_t get_random(uint32_t limit);

void memset(volatile void* addr, uint8_t value, uint64_t size);
void* memcpy(void *dest, const void *src, size_t n);
int streq(char* a, char* b);
int memeq(void* a1, void* b1, int64_t size);

void panic_halt();

void debug_wait();

#endif