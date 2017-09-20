#include <sys/defs.h>

#ifndef _MISC_H
#define _MISC_H

extern uint64_t pic_tick_count;
extern uint8_t last_key_pressed[];

void asm_outb(uint16_t port, uint8_t value);
uint8_t asm_inb(uint16_t port);

void update_topright_display();
uint32_t get_random(uint32_t limit);

#endif