



#ifndef _KTHREAD_H
#define _KTHREAD_H

typedef struct Process Process;
typedef struct m_map m_map;

#include <sys/elf64.h>
#include <sys/memory/phy_page.h>
#include <sys/idt.h>

#define USER_STACK_SEGMENT_SELECTOR 0x23
#define USER_CODE_SEGMENT_SELECTOR 0x1b
#define PROCESS_INITIAL_RSP 0xFFFFFFFF7FFFFFE0

struct Process{
	uint32_t id;
	char* name;
	uint64_t cr3;
	struct Process* next;
	uint64_t rsp;
	uint64_t rip;
	reg_saved reg;
	uint64_t rsp_current;
	uint64_t heap_start;
	uint64_t heap_break;
	unsigned char on_hold: 1;
	unsigned char terminated: 1;
	unsigned char cleaned: 1;
	uint64_t ret_value;
	m_map* first_map;
};

struct m_map{
	char type; // 0: undefined, 1: program page table, 2: program accesible page, 3:pml4 page table
	page_entry* phy_page;
	uint64_t vir_addr;
	Process* proc;
	m_map* next;
	unsigned char shared: 1;// if this page share this page with someone else
	unsigned char rw: 1; // if this page is actually can be written by process
};

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

void* add_page_for_process(Process* proc, uint64_t new_address, char rw);
void spawn_process(program_section* section, char* elf_file_path);

Process* fork_process(Process* parent);

void process_cleanup(Process* proc);

int check_and_handle_rw_page_fault(Process* proc, uint64_t addr/* accessed address */);

void add_kernel_thread_to_process_list();

void process_schedule();
void switch_process();
void save_current_state(handler_reg volatile reg);
void save_previous_rsp(uint64_t rsp);
void save_previous_rip(uint64_t rip);
void save_previous_registers(handler_reg volatile reg);
uint64_t load_current_rsp();
uint64_t load_current_cr3();
void load_previous_registers(handler_reg volatile reg);

#endif