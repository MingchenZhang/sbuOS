#ifndef _IDT_H
#define _IDT_H

#include <sys/defs.h>

void init_idt();
void init_pic();

struct handler_reg{
	uint64_t rbp, r9, r8, rax, rcx, rdx, rbx, rsi, rdi;
	uint64_t int_num, err_num;
} __attribute__((packed)); 
typedef struct handler_reg handler_reg;

void isr_handler(handler_reg);

#endif