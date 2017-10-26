

#include <sys/elf64.h>
#include <sys/idt.h>

#ifndef _KTHREAD_H
#define _KTHREAD_H

#define USER_STACK_SEGMENT_SELECTOR 0x23
#define USER_CODE_SEGMENT_SELECTOR 0x1b

struct Process{
	uint32_t id;
	char* name;
	uint64_t cr3;
	struct Process* next;
	uint64_t rsp;
	uint64_t rip;
	handler_reg reg;
	char on_hold;
};
typedef struct Process Process;

struct Process_init{
	uint64_t ins_start;
	uint64_t ins_size;
};
typedef struct Process_init Process_init;

extern Process* first_process;
extern Process* current_process;
extern Process* previous_process;

void test_spawn_process(void* initial_stack, uint64_t stack_size);
void test_spawn_process_elf(program_section* section, char* elf_file_path);
void add_kernel_thread_to_process_list();
void process_schedule();
void switch_process();
void save_previous_rsp(uint64_t rsp);
void save_previous_rip(uint64_t rip);
void save_previous_registers(handler_reg volatile reg);
uint64_t load_current_rsp();
uint64_t load_current_cr3();
void load_previous_registers(handler_reg volatile reg);

#endif