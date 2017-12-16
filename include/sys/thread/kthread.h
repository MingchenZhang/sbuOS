



#ifndef _KTHREAD_H
#define _KTHREAD_H

typedef struct Process Process;
typedef struct m_map m_map;

#include <sys/elf64.h>
#include <sys/memory/phy_page.h>
#include <sys/idt.h>
#include <sys/disk/file_system.h>
#include <sys/elf64.h>

#define FD_SIZE 16

#define USER_STACK_SEGMENT_SELECTOR 0x23
#define USER_CODE_SEGMENT_SELECTOR 0x1b
#define KERNEL_STACK_SEGMENT_SELECTOR 0x10
#define KERNEL_CODE_SEGMENT_SELECTOR 0x08
#define EFLAG_NO_INTERRUPT 0x200046
#define EFLAG_INTERRUPT 0x200216
#define RPOCESS_RSP0_SIZE 4096

typedef struct open_file_descriptor {
	file_table_entry* file_entry;
} open_file_descriptor;

struct Process{
	uint32_t id;
	Process* parent;
	char* name;
	uint64_t cr3;
	struct Process* next;
	uint64_t rsp;
	uint64_t rip;
	handler_reg saved_reg;
	uint64_t rsp_current; // location of current stack boundary (to identify page fault near stack boundary)
	uint64_t heap_start;
	uint64_t heap_break;
	uint64_t on_hold: 1;
	uint64_t terminated: 1;
	uint64_t cleaned: 1;
	int ret_value;
	m_map* first_map;
	open_file_descriptor* open_fd[FD_SIZE];
	char* workdir;
	uint64_t sig_pending;
	uint64_t sig_handler;
	handler_reg sig_saved_reg; // sig_saved_reg.int_num is 0x80 if the process is in signalled state, otherwise it should be zero
	int32_t id_wait_for;
	Process* id_wait_for_p;
};

struct m_map{
	char type; // 0: undefined, 1: program page table, 2: program accesible page, 3:kernel page table, 4: rsp0 stack
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

void* process_rsp0_start;
void* process_initial_rsp;

void init_process();

void* add_page_for_process(Process* proc, uint64_t new_address, char rw, char map_type);
void spawn_process(program_section* section, char* elf_file_path);

Process* fork_process(Process* parent);

void replace_process(Process* proc, program_section* section, uint64_t* initial_stack, uint64_t initial_stack_size);

void process_cleanup(Process* proc);

int check_and_handle_rw_page_fault(Process* proc, uint64_t addr/* accessed address */);

void add_kernel_thread_to_process_list();


int process_add_signal(uint32_t pid, uint64_t signal);
Process* search_process(uint32_t pid);
void process_schedule();
void switch_process();
void save_current_state(handler_reg volatile reg);
void save_current_cr3(uint64_t cr3);
void save_previous_rsp(uint64_t rsp);
void save_previous_rip(uint64_t rip);
uint64_t load_current_rsp();
uint64_t load_current_cr3();
uint64_t get_rsp0_stack();
uint64_t get_kernel_cr3();

#endif