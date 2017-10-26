#include <sys/kprintf.h>
#include <sys/defs.h>
#include <sys/memory/phy_page.h>
#include <sys/memory/kmalloc.h>
#include <sys/misc.h>
#include <sys/thread/kthread.h>
#include <sys/elf64.h>
#include <sys/tarfs.h>
#include <sys/idt.h>

#define EFLAGS_PROCESS 0x200216 // 0x200046 for no interrupt, 0x200216 for interrupt

Process* first_process = 0;
Process* current_process = 0;
Process* previous_process = 0;
uint32_t id_count = 0;

void add_page_to_pt(uint64_t pml4, uint64_t new_address, uint64_t points_to){
	register uint64_t* pml4e = &(((uint64_t*)(pml4 & 0xFFFFFFFFFF000))[(new_address>>39) & 0b111111111]);
	if((*pml4e & 0x1) == 0){
		PDPE* pdp = get_phy_page(1, 3);
		memset(pdp, 0, 4096);
		PML4E* pml4e_ = (PML4E*)pml4e;
		pml4e_->P = 1;
		pml4e_->RW = 1;
		pml4e_->US = 1;
		pml4e_->PDPE_addr = (uint64_t)pdp >> 12;
	}
	register uint64_t* pdpe = &(((uint64_t*)(*pml4e & 0xFFFFFFFFFF000))[(new_address>>30) & 0b111111111]);
	if((*pdpe & 0x1) == 0){
		PDPE* pd = get_phy_page(1, 3);
		memset(pd, 0, 4096);
		PDPE* pdpe_ = (PDPE*)pdpe;
		pdpe_->P = 1;
		pdpe_->RW = 1;
		pdpe_->US = 1;
		pdpe_->PS = 0;
		pdpe_->PDPE_addr = (uint64_t)pd >> 12;
	}
	register uint64_t* pde = &(((uint64_t*)(*pdpe & 0xFFFFFFFFFF000))[(new_address>>21) & 0b111111111]);
	if((*pde & 0x1) == 0){
		PDPE* pt = get_phy_page(1, 3);
		memset(pt, 0, 4096);
		PDE* pde_ = (PDE*)pde;
		pde_->P = 1;
		pde_->RW = 1;
		pde_->US = 1;
		pde_->PS = 0;
		pde_->PDPE_addr = (uint64_t)pt >> 12;
	}
	register uint64_t* pte = &(((uint64_t*)(*pde & 0xFFFFFFFFFF000))[(new_address>>12) & 0b111111111]);
	if((*pte & 0x1) == 0){
		PTE* pte_ = (PTE*)pte;
		pte_->P = 1;
		pte_->RW = 1;
		pte_->US = 1;
		pte_->PDPE_addr = (uint64_t)points_to >> 12;
	}
}

uint64_t create_process_page_table_elf(program_section* section, char* elf_file_path){
	PML4E* PML4 = get_phy_page(1, 3);
	PDPE* PDP1 = get_phy_page(1, 3);
	memset(PML4, 0, 4096);
	memset(PDP1, 0, 4096);
	
	PML4E* pml1 = PML4 + 511; // upper pml4 entry
	pml1->P = 1;
	pml1->RW = 1;
	pml1->US = 1;
	pml1->PDPE_addr = (uint64_t)PDP1 >> 12;
	
	PDPE* pdp1 = PDP1 + 511; // upper pdp entry
	pdp1->P = 1;
	pdp1->RW = 1;
	pdp1->US = 0;
	pdp1->PS = 0;
	pdp1->PDPE_addr = (uint64_t)kernel_base_pd >> 12; // points to the start of the memory
	
	PDPE* pdp2 = PDP1 + 510; // kmalloc pdp entry
	pdp2->P = 1;
	pdp2->RW = 1;
	pdp2->US = 0;
	pdp2->PS = 0;
	pdp2->PDPE_addr = (uint64_t)kernel_malloc_pd >> 12; // points to the start of the memory
	
	add_page_to_pt((uint64_t)PML4, 0xFFFFFFFF7FFFFFE0, (uint64_t)get_phy_page(1, 3)); // just alloc one page stack
	
	// alloc data pages
	while(section){
		uint64_t progress = 0;
		for(;progress < section->size; progress += 4096){
			void* new_page = get_phy_page(1, 3);
			int32_t to_read = 4096;
			if(section->size - progress < 4096) to_read = section->size - progress;
			if(tarfs_read(elf_file_path, new_page, to_read, progress) < to_read){
				kprintf("ERROR: cannot read enough segment from elf file\n");
				while(1); // TODO: clean up and return instead of hung
			}
			add_page_to_pt((uint64_t) PML4, section->memory_offset+progress, (uint64_t) new_page);
		}
		section = section->next;
	}
	
	return (uint64_t)PML4;
}

void test_spawn_process_elf(program_section* section, char* elf_file_path){
	kprintf("DEBUG: start generating page table\n");
	uint64_t cr3 = create_process_page_table_elf(section, elf_file_path);
	
	uint64_t* stack_start = (uint64_t*)(translate_cr3(cr3, 0xFFFFFFFF7FFFFFE0));
	*(stack_start - 1) = USER_STACK_SEGMENT_SELECTOR; // stack segment selector
	*(stack_start - 2) = (uint64_t)0xFFFFFFFF7FFFFFE0; // where stack starts
	*(stack_start - 3) = EFLAGS_PROCESS;  // EFLAGS
	*(stack_start - 4) = USER_CODE_SEGMENT_SELECTOR; // code segment seletor
	*(stack_start - 5) = section->entry_point; // where ip starts
	*(stack_start - 6) = 0;	// error code
	*(stack_start - 7) = 0x80; // interrupt number
	Process* new_p = sf_malloc(sizeof(Process));
	new_p->id = id_count++;
	new_p->name = "test elf process";
	new_p->next = 0;
	new_p->cr3 = cr3;
	new_p->rsp = (uint64_t)(0xFFFFFFFF7FFFFFE0 - 8*16);
	new_p->rip = section->entry_point;
	// attach new process to the end
	if(!first_process) first_process = new_p;
	else{
		Process* cursor = first_process;
		while(cursor->next){
			cursor = cursor->next;
		}
		cursor->next = new_p;
	}
}

void add_kernel_thread_to_process_list(){
	uint64_t cr3;
	__asm__ volatile ("movq %%cr3, %0;":"=r"(cr3));
	cr3 &= 0xFFFFFFFFFFFFF000;
	Process* new_p = sf_malloc(sizeof(Process));
	new_p->id = id_count++;
	new_p->name = "kernel process";
	new_p->next = 0;
	new_p->cr3 = cr3;
	new_p->rsp = (uint64_t)(0); // undefined until tobe switched out
	// attach new process to the end
	if(!first_process) first_process = new_p;
	else{
		Process* cursor = first_process;
		while(cursor->next){
			cursor = cursor->next;
		}
		cursor->next = new_p;
	}
	current_process = new_p;
}

void process_schedule(){
	Process* next;
	// if(!current_process){
		// next = first_process;
	// }else{
		// if(current_process->next) next = current_process->next;
		// else next = first_process;
	// }
	if(!current_process) next = first_process;
	else if(current_process->next) next = current_process->next;
	else next = first_process;
	while(1){
		if(next == current_process){
			break;
		}
		if(!next->on_hold){
			break;
		}
		next = next->next;
		if(!next) next = first_process;
	}
	if(!next){
		kprintf("PANIC: no process to schedule\n");
		panic_halt();
	}
	if(next->on_hold){
		kprintf("PANIC: all process on hold\n");
		panic_halt();
	}
	// kprintf("process_schedule next: %p\n", next);
	previous_process = current_process;
	current_process = next;
}

void save_previous_rsp(uint64_t rsp){
	if(previous_process) previous_process->rsp = rsp;
}

void save_previous_rip(uint64_t rip){
	if(previous_process) previous_process->rip = rip;
}

void save_previous_registers(handler_reg volatile reg){
	if(previous_process) {
		previous_process->reg.rbp = reg.rbp;
		previous_process->reg.r9 = reg.r9;
		previous_process->reg.r8 = reg.r8;
		previous_process->reg.rax = reg.rax;
		previous_process->reg.rcx = reg.rcx;
		previous_process->reg.rdx = reg.rdx;
		previous_process->reg.rbx = reg.rbx;
		previous_process->reg.rsi = reg.rsi;
		previous_process->reg.rdi = reg.rdi;
	}
}

uint64_t load_current_rsp(){
	return current_process->rsp;
}

uint64_t load_current_cr3(){
	return current_process->cr3;
}

uint64_t load_current_rip(){
	return current_process->rip;
}

void load_previous_registers(handler_reg volatile reg){
	if(current_process) {
		reg.rbp = current_process->reg.rbp;
		reg.r9 = current_process->reg.r9;
		reg.r8 = current_process->reg.r8;
		reg.rax = current_process->reg.rax;
		reg.rcx = current_process->reg.rcx;
		reg.rdx = current_process->reg.rdx;
		reg.rbx = current_process->reg.rbx;
		reg.rsi = current_process->reg.rsi;
		reg.rdi = current_process->reg.rdi;
	}
}