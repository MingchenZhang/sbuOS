#ifndef _IDT_H
#define _IDT_H

#include <sys/defs.h>

void init_idt();
void init_pic();

struct handler_reg{
	uint64_t rbp, r15, r14, r13, r12, r9, r8, rax, rcx, rdx, rbx, rsi, rdi;
	uint64_t int_num, err_num, ret_rip, cs, eflags, ret_rsp, ss;
} __attribute__((packed)); 
typedef struct handler_reg handler_reg;

// argument order: rdi, rsi, rdx, rcx, r8, r9

typedef struct reg_saved reg_saved;
struct reg_saved{
	uint64_t rbp, r9, r8, rax, rcx, rdx, rbx, rsi, rdi;
};

struct kernel_task_return_reg{
	uint64_t cr3, rsp;
};

typedef struct kernel_space_task{
	int type;
	uint64_t param[6];
	uint64_t ret[6];
} kernel_space_task;
kernel_space_task kernel_space_task_file;

#define TASK_TIMER_TICK 1
#define TASK_FORK_PROCESS 2
#define TASK_REG_WAIT 3
#define TASK_PROC_PAGE_FAULT_HANDLE 4
#define TASK_PROC_CLEANUP 5
#define TASK_KEYBOARD_HANDLE 6
#define TASK_KBRK 7
#define TASK_REPLACE_PROCESS 8
#define TASK_RW_DISK_BLOCK 9
#define TASK_CP_PAGE_MALLOC 10

int64_t isr_handler(handler_reg);

extern void kernel_space_handler_wrapper();

void kernel_space_handler();

#endif