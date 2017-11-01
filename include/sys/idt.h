#ifndef _IDT_H
#define _IDT_H

#include <sys/defs.h>

void init_idt();
void init_pic();

struct handler_reg{
	uint64_t rbp, r9, r8, rax, rcx, rdx, rbx, rsi, rdi;
	uint64_t int_num, err_num, ret_rip, cs, eflags, ret_rsp, ss;
} __attribute__((packed)); 
typedef struct handler_reg handler_reg;

// argument order: rdi, rsi, rdx, rcx, r8, r9

typedef struct reg_saved reg_saved;
struct reg_saved{
	uint64_t rbp, r9, r8, rax, rcx, rdx, rbx, rsi, rdi;
};

void isr_handler(handler_reg);

#endif