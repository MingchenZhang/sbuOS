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
	
	add_page_to_pt((uint64_t)PML4, PROCESS_INITIAL_RSP, (uint64_t)get_phy_page(1, 3)); // just alloc one page stack
	
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
	
	uint64_t* stack_start = (uint64_t*)(translate_cr3(cr3, PROCESS_INITIAL_RSP));
	*(stack_start - 1) = USER_STACK_SEGMENT_SELECTOR; // stack segment selector
	*(stack_start - 2) = (uint64_t)PROCESS_INITIAL_RSP; // where stack starts
	*(stack_start - 3) = EFLAGS_PROCESS;  // EFLAGS
	*(stack_start - 4) = USER_CODE_SEGMENT_SELECTOR; // code segment seletor
	*(stack_start - 5) = section->entry_point; // where ip starts
	*(stack_start - 6) = 0;	// error code
	*(stack_start - 7) = 0x80; // interrupt number
	Process* new_p = sf_calloc(sizeof(Process), 1);
	new_p->id = id_count++;
	new_p->name = "test elf process";
	new_p->next = 0;
	new_p->cr3 = cr3;
	new_p->rsp = (uint64_t)(PROCESS_INITIAL_RSP - 8*16);
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

m_map* add_new_m_map_for_process(Process* proc, char type, char shared, char rw, uint64_t vir_addr){
	m_map* map = sf_calloc(sizeof(m_map), 1);
	map->type = type;
	map->proc = proc;
	map->shared = shared;
	map->rw = rw;
	map->phy_page = get_phy_page_for_program(proc, map);
	map->vir_addr = vir_addr;
	map->next = 0;
	m_map* cursor = proc->first_map;
	if(!cursor){
		proc->first_map = map;
		return map;
	}
	while(cursor->next) {
		cursor = cursor->next;
	}
	cursor->next = map;
	return map;
}

m_map* dup_new_m_map_for_process(Process* proc/* dest proc */, char type, char shared, char rw, uint64_t vir_addr, page_entry* old_page){
	m_map* map = sf_calloc(sizeof(m_map), 1);
	map->type = type;
	map->proc = proc;
	map->shared = shared;
	map->rw = rw;
	dup_page_for_program(old_page, map);
	map->phy_page = old_page;
	map->vir_addr = vir_addr;
	map->next = 0;
	m_map* cursor = proc->first_map;
	if(!cursor){
		proc->first_map = map;
		return map;
	}
	while(cursor->next) {
		cursor = cursor->next;
	}
	cursor->next = map;
	return map;
}

void* add_page_for_process(Process* proc, uint64_t new_address, char rw){
	uint64_t pml4 = proc->cr3;
	register uint64_t* pml4e = &(((uint64_t*)(pml4 & 0xFFFFFFFFFF000))[(new_address>>39) & 0b111111111]);
	if((*pml4e & 0x1) == 0){
		PDPE* pdp = add_new_m_map_for_process(proc, 1, 0, 0, 0)->phy_page->base;
		memset(pdp, 0, 4096);
		PML4E* pml4e_ = (PML4E*)pml4e;
		pml4e_->P = 1;
		pml4e_->RW = 1;
		pml4e_->US = 1;
		pml4e_->PDPE_addr = (uint64_t)pdp >> 12;
	}
	register uint64_t* pdpe = &(((uint64_t*)(*pml4e & 0xFFFFFFFFFF000))[(new_address>>30) & 0b111111111]);
	if((*pdpe & 0x1) == 0){
		PDPE* pd = add_new_m_map_for_process(proc, 1, 0, 0, 0)->phy_page->base;
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
		PDPE* pt = add_new_m_map_for_process(proc, 1, 0, 0, 0)->phy_page->base;
		memset(pt, 0, 4096);
		PDE* pde_ = (PDE*)pde;
		pde_->P = 1;
		pde_->RW = 1;
		pde_->US = 1;
		pde_->PS = 0;
		pde_->PDPE_addr = (uint64_t)pt >> 12;
	}
	register uint64_t* pte = &(((uint64_t*)(*pde & 0xFFFFFFFFFF000))[(new_address>>12) & 0b111111111]);
	void* dest = add_new_m_map_for_process(proc, 2, 0, rw, new_address)->phy_page->base;
	PTE* pte_ = (PTE*)pte;
	pte_->P = 1;
	pte_->RW = rw;
	pte_->US = 1;
	pte_->PDPE_addr = (uint64_t)dest >> 12;
	return dest;
}

void spawn_process(program_section* section, char* elf_file_path){
	Process* new_p = sf_calloc(sizeof(Process), 1);
	
	m_map* pml4_map = sf_calloc(sizeof(m_map), 1);
	pml4_map->type = 3;
	pml4_map->proc = new_p;
	pml4_map->shared = 0;
	pml4_map->rw = 0;
	pml4_map->phy_page = get_phy_page_for_program(new_p, pml4_map);
	PML4E* PML4 = pml4_map->phy_page->base;
	
	new_p->cr3 = (uint64_t)PML4;
	new_p->first_map = pml4_map;
	
	m_map* pdp1_map = sf_calloc(sizeof(m_map), 1);
	pml4_map->next = pdp1_map;
	pdp1_map->type = 1;
	pdp1_map->proc = new_p;
	pdp1_map->shared = 0;
	pdp1_map->rw = 0;
	pdp1_map->phy_page = get_phy_page_for_program(new_p, pdp1_map);
	PDPE* PDP1 = pdp1_map->phy_page->base;
	
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
	
	//create one page for the stack now
	add_page_for_process(new_p, PROCESS_INITIAL_RSP, 1);
	//add_page_to_pt((uint64_t)PML4, PROCESS_INITIAL_RSP, (uint64_t)get_phy_page(1, 3)); // just alloc one page stack
	
	// alloc data pages
	program_section* section_cursor = section;
	uint64_t section_height_max = 0;
	while(section_cursor){
		// section_cursor->file_offset
		// section_cursor->memory_offset
		// section_cursor->size
		// section_cursor->entry_point
		uint64_t prog_disk = section_cursor->file_offset;
		uint64_t prog_mem = section_cursor->memory_offset;
		uint64_t to_read = 0;
		uint64_t remain = section_cursor->size;
		for(; remain != 0; 
			prog_disk += to_read, prog_mem += to_read, remain -= to_read){
			to_read = math_min(4096, 4096-(prog_mem % 4096), remain, (uint64_t)-1);
			void* new_page = add_page_for_process(new_p, prog_mem, 1);
			if(tarfs_read(elf_file_path, new_page+(prog_mem%4096), to_read, prog_disk) < to_read){
				kprintf("ERROR: cannot read enough segment from elf file\n");
				while(1); // TODO: clean up and return instead of hung
			}
		}
		// for(uint64_t progress = 0; progress < section_cursor->size; progress += 4096){
			// uint64_t section_h = section_cursor->memory_offset+section_cursor->size;
			// if(section_h>section_height_max) section_height_max = section_h;
			// void* new_page = add_page_for_process(new_p, section_cursor->memory_offset+progress, 1);
			// int32_t to_read = 4096;
			// if(section_cursor->size - progress < 4096) to_read = section_cursor->size - progress;
			// if(tarfs_read(elf_file_path, new_page, to_read, progress) < to_read){
				// kprintf("ERROR: cannot read enough segment from elf file\n");
				// while(1); // TODO: clean up and return instead of hung
			// }
		// }
		section_cursor = section_cursor->next;
	}
	
	// now the address space is set up
	
	new_p->id = id_count++;
	new_p->name = "elf process";
	new_p->next = 0;
	new_p->cr3 = (uint64_t)PML4;
	new_p->rsp = (uint64_t)(PROCESS_INITIAL_RSP);
	new_p->rip = section->entry_point;
	new_p->rsp_current = (uint64_t)(PROCESS_INITIAL_RSP);
	new_p->heap_start = section_height_max;
	new_p->heap_break = section_height_max;
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

void* dup_page_for_process(Process* proc_dest, uint64_t new_address, char rw, page_entry* ptr_to){
	uint64_t pml4 = proc_dest->cr3;
	register uint64_t* pml4e = &(((uint64_t*)(pml4 & 0xFFFFFFFFFF000))[(new_address>>39) & 0b111111111]);
	if((*pml4e & 0x1) == 0){
		PDPE* pdp = add_new_m_map_for_process(proc_dest, 1, 0, 0, 0)->phy_page->base;
		memset(pdp, 0, 4096);
		PML4E* pml4e_ = (PML4E*)pml4e;
		pml4e_->P = 1;
		pml4e_->RW = 1;
		pml4e_->US = 1;
		pml4e_->PDPE_addr = (uint64_t)pdp >> 12;
	}
	register uint64_t* pdpe = &(((uint64_t*)(*pml4e & 0xFFFFFFFFFF000))[(new_address>>30) & 0b111111111]);
	if((*pdpe & 0x1) == 0){
		PDPE* pd = add_new_m_map_for_process(proc_dest, 1, 0, 0, 0)->phy_page->base;
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
		PDPE* pt = add_new_m_map_for_process(proc_dest, 1, 0, 0, 0)->phy_page->base;
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
		void* dest = dup_new_m_map_for_process(proc_dest, 2, 1, rw, new_address, ptr_to)->phy_page->base;
		PTE* pte_ = (PTE*)pte;
		pte_->P = 1;
		pte_->RW = 0;
		pte_->US = 1;
		pte_->PDPE_addr = (uint64_t)dest >> 12;
		return dest;
	}
	return (uint64_t*)(uint64_t)(((PTE*)pte)->PDPE_addr << 12);
}

Process* fork_process(Process* parent){
	// before calling, parent process state must be written back (rip, rsp)
	Process* new_p = sf_calloc(sizeof(Process), 1);
	
	m_map* pml4_map = sf_calloc(sizeof(m_map), 1);
	pml4_map->type = 3;
	pml4_map->proc = new_p;
	pml4_map->shared = 0;
	pml4_map->rw = 0;
	pml4_map->phy_page = get_phy_page_for_program(new_p, pml4_map);
	PML4E* PML4 = pml4_map->phy_page->base;
	
	new_p->cr3 = (uint64_t)PML4;
	new_p->first_map = pml4_map;
	
	m_map* pdp1_map = sf_calloc(sizeof(m_map), 1);
	pml4_map->next = pdp1_map;
	pdp1_map->type = 1;
	pdp1_map->proc = new_p;
	pdp1_map->shared = 0;
	pdp1_map->rw = 0;
	pdp1_map->phy_page = get_phy_page_for_program(new_p, pdp1_map);
	PDPE* PDP1 = pdp1_map->phy_page->base;
	
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
	
	m_map* map_cur = parent->first_map;
	while(map_cur){
		if(map_cur->type == 2){
			assert(set_pte_rw(parent->cr3, map_cur->vir_addr, 0), "set_pte_rw: process page table not reach pte\n");
			dup_page_for_process(new_p, map_cur->vir_addr, map_cur->rw, map_cur->phy_page);
		}
		map_cur = map_cur->next;
	}
	
	new_p->id = id_count++;
	new_p->name = "elf process";
	new_p->next = 0;
	new_p->cr3 = (uint64_t)PML4;
	new_p->rsp = parent->rsp;
	new_p->rip = parent->rip;
	new_p->reg = parent->reg;
	new_p->rsp_current = parent->rsp_current;
	new_p->heap_start = parent->heap_start;
	new_p->heap_break = parent->heap_break;
	new_p->on_hold = 0;
	new_p->reg.rax = 0; // forked process has a return value of 0
	// attach new process to the end
	if(!first_process) first_process = new_p;
	else{
		Process* cursor = first_process;
		while(cursor->next){
			cursor = cursor->next;
		}
		cursor->next = new_p;
	}
	
	return new_p;
}

void process_cleanup(Process* proc){
	m_map* map_cur = proc->first_map;
	while(map_cur){
		free_page_for_program(map_cur->phy_page, map_cur);
		map_cur = map_cur->next;
	}
	
	map_cur = proc->first_map;
	while(map_cur){
		m_map* next = map_cur->next;
		sf_free(map_cur);
		map_cur = next;
	}
	
	// now remove process from schedule list
	if(first_process == proc){
		first_process = proc->next;
	}else{
		Process* proc_c = first_process;
		while(proc_c->next){
			if(proc_c->next == proc){
				proc_c->next = proc->next;
			}else{
				proc_c = proc_c->next;
			}
		}
	}
	
	// now proc still exist with everything reclaimed, it is a zombie now. 
	proc->cleaned = 1;
}

int check_and_handle_rw_page_fault(Process* proc, uint64_t addr/* accessed address */){
	m_map* map_cur = proc->first_map;
	while(map_cur->next){
		m_map* curr_map = map_cur->next;
		if((curr_map->type == 2) && (curr_map->vir_addr>>12 == addr>>12)){
			if(curr_map->rw){
				void* new_page = add_page_for_process(proc, addr, 1);
				memcpy(new_page, curr_map->phy_page, 4096);
				free_page_for_program(curr_map->phy_page, curr_map);
				map_cur->next = map_cur->next->next;
				return 1;
			}else{
				return 0;
			}
		}
		map_cur = map_cur->next;
	}
	return 0;
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
		if(next->terminated && !next->cleaned){
			kprintf("DEBUG: cleaning proc id:%d\n", next->id);
			process_cleanup(next);
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

void save_current_state(handler_reg volatile reg){
	if(current_process){
		current_process->rsp = reg.ret_rsp;
		current_process->rip = reg.ret_rip;
	}
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