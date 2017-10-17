#ifndef _KTHREAD_H
#define _KTHREAD_H

struct Process{
	uint32_t id;
	char* name;
	uint64_t cr3;
	struct Process* next;
	uint64_t rsp;
};
typedef struct Process Process;

struct Process_init{
	uint64_t ins_start;
	uint64_t ins_size;
};
typedef struct Process_init Process_init;

void test_spawn_process(void* initial_stack, uint64_t stack_size);
void process_schedule();
void switch_process();
void save_previous_rsp(uint64_t rsp);
uint64_t load_current_rsp();
uint64_t load_current_cr3();

#endif