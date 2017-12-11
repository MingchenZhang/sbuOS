#include <sys/kprintf.h>
#include <sys/defs.h>
#include <sys/memory/phy_page.h>
#include <sys/memory/kmalloc.h>
#include <sys/misc.h>
#include <sys/thread/kthread.h>
#include <sys/elf64.h>
#include <sys/tarfs.h>
#include <sys/idt.h>
#include <sys/gdt.h>
#include <sys/terminal.h>

#define EFLAGS_PROCESS 0x200216 // 0x200046 for no interrupt, 0x200216 for interrupt

Process* first_process = 0;
Process* current_process = 0;
Process* previous_process = 0;
uint32_t id_count = 0;

#define RSP0_STACK_SIZE 4096
uint8_t rsp0_stack[RSP0_STACK_SIZE]__attribute__((aligned(16))); // used as a clean stack to switch back to ring0

extern void AFTER_CONTEXT_SWITCH();

void init_process(){
	process_rsp0_start = process_direct_mapping_addr - RPOCESS_RSP0_SIZE;
	process_initial_rsp = process_rsp0_start;
	set_tss_rsp((void*)process_rsp0_start + RPOCESS_RSP0_SIZE - 16); 
}

m_map* add_new_m_map_for_process(Process* proc, char type, char shared, char rw, uint64_t vir_addr){
	m_map* map = sf_calloc(sizeof(m_map), 1);
	map->type = type;
	map->proc = proc;
	map->shared = shared;
	map->rw = rw;
	map->phy_page = get_phy_page_for_program(proc, map);
	memset(map->phy_page->base, 0, 4096);
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

void* add_page_for_process(Process* proc, uint64_t new_address, char rw, char map_type){
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
	PTE* pte_ = (PTE*)pte;
	void* dest;
	if(pte_->P == 1 && pte_->RW == 1){
		kprintf("WARNING: process page reallocated\n");
	}
	dest = add_new_m_map_for_process(proc, map_type, 0, rw, new_address)->phy_page->base;
	pte_->P = 1;
	pte_->RW = rw;
	pte_->US = 1;
	pte_->PDPE_addr = (uint64_t)dest >> 12;
	return dest;
}

void* map_page_for_process(Process* proc, uint64_t new_address, char rw, char map_type, uint64_t points_to){
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
	void* dest = (void*)points_to;
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
	
	PML4E* pml2 = PML4 + 510; // upper pml4 entry
	pml2->P = 1;
	pml2->RW = 1;
	pml2->US = 0;
	pml2->PDPE_addr = (uint64_t)0 >> 12; // a simple direct mapping
	
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
	
	// create one page for the stack now
	assert(RPOCESS_RSP0_SIZE == 4096, "ERROR: only 4096 rsp0 stack supported\n");
	void* actual_rsp0 = add_page_for_process(new_p, (uint64_t)process_rsp0_start, 1, 4);
	add_page_for_process(new_p, (uint64_t)process_initial_rsp - 16, 1, 2);
	
	// alloc data pages
	program_section* section_cursor = section;
	uint64_t section_height_max = 0;
	while(section_cursor){
		uint64_t prog_disk = section_cursor->file_offset;
		uint64_t prog_mem = section_cursor->memory_offset;
		uint64_t to_read = 0;
		uint64_t remain = section_cursor->size;
		uint64_t mem_remain = section_cursor->mem_size;
		void* new_page = 0;
		for(; remain != 0; 
			prog_disk += to_read, prog_mem += to_read, remain -= to_read, mem_remain -= to_read){
			to_read = math_min(4096, 4096-(prog_mem % 4096), remain, (uint64_t)-1);
			new_page = add_page_for_process(new_p, prog_mem, 1, 2);
			if(tarfs_read(elf_file_path, new_page+(prog_mem%4096), to_read, prog_disk) < to_read){
				kprintf("ERROR: cannot read enough segment from elf file\n");
				while(1); // TODO: clean up and return instead of hung
			}
		}
		if(mem_remain != 0) {
			for(; mem_remain != 0; prog_mem++, mem_remain--){
				if(prog_mem % 0x1000 == 0){
					new_page = add_page_for_process(new_p, prog_mem, 1, 2);
				}
				*(uint8_t*)(uint64_t*)(new_page+(prog_mem%4096)) = 0;
			}
		}
		section_cursor = section_cursor->next;
	}
	
	// now the address space is set up
	
	handler_reg* reg = (void*)(actual_rsp0 + RPOCESS_RSP0_SIZE - 16 - sizeof(handler_reg));// does not support multuple rsp0 pages
	handler_reg* reg2 = reg - 1;
	new_p->id = id_count++;
	new_p->name = "elf process";
	new_p->next = 0;
	new_p->cr3 = (uint64_t)PML4;
	new_p->rsp = (uint64_t)(process_rsp0_start + RPOCESS_RSP0_SIZE - 16- 2*sizeof(handler_reg));
	new_p->rsp_current = (uint64_t)(process_initial_rsp);
	new_p->heap_start = section_height_max;
	new_p->heap_break = section_height_max;
	new_p->workdir = sf_malloc(5);
	memcpy(new_p->workdir, "/bin", 5);
	// give basic file entries
	file_table_entry* stdin_file = terminal_file_out[0];
	file_table_entry* stdout_file = terminal_file_in[1];
	stdin_file->open_count++;
	stdout_file->open_count++;
	open_file_descriptor* stdin_fd = sf_malloc(sizeof(open_file_descriptor));
	stdin_fd->file_entry = stdin_file;
	open_file_descriptor* stdout_fd = sf_malloc(sizeof(open_file_descriptor));
	stdout_fd->file_entry = stdout_file;
	new_p->open_fd[0] = stdin_fd;
	new_p->open_fd[1] = stdout_fd;
	// setup basic rsp0 content
	memset(reg, 0, sizeof(handler_reg));
	reg->cs = USER_CODE_SEGMENT_SELECTOR;
	reg->ss = USER_STACK_SEGMENT_SELECTOR;
	reg->eflags = EFLAG_INTERRUPT; // enable interrupt
	reg->ret_rsp = (uint64_t)(process_initial_rsp - 16);
	reg->ret_rip = section->entry_point;
	// setup basic rsp0 context switch content
	memset(reg2, 0, sizeof(handler_reg));
	reg2->cs = KERNEL_CODE_SEGMENT_SELECTOR;
	reg2->ss = KERNEL_STACK_SEGMENT_SELECTOR;
	reg2->eflags = EFLAG_NO_INTERRUPT; // disable interrupt
	reg2->ret_rsp = (uint64_t)(process_rsp0_start + RPOCESS_RSP0_SIZE - 16- sizeof(handler_reg));
	reg2->ret_rip = (uint64_t)AFTER_CONTEXT_SWITCH;
	// attach new process to the end
	if(!first_process) first_process = new_p;
	else{
		Process* cursor = first_process;
		while(cursor->next){
			cursor = cursor->next;
		}
		cursor->next = new_p;
	}
	kprintf("DEBUG: thread spawned, cr3: %x\n", PML4);
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
	
	PML4E* pml2 = PML4 + 510; // upper pml4 entry
	pml2->P = 1;
	pml2->RW = 1;
	pml2->US = 0;
	pml2->PDPE_addr = (uint64_t)0 >> 12; // a simple direct mapping
	
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
	
	// create rsp0 stack for this process
	assert(RPOCESS_RSP0_SIZE == 4096, "ERROR: only 4096 rsp0 stack supported\n");
	void* actual_rsp0 = add_page_for_process(new_p, (uint64_t)process_rsp0_start, 1, 4);
	
	m_map* map_cur = parent->first_map;
	while(map_cur){
		if(map_cur->type == 2){
			assert(set_pte_rw(parent->cr3, map_cur->vir_addr, 0), "set_pte_rw: process page table not reach pte\n");
			dup_page_for_process(new_p, map_cur->vir_addr, map_cur->rw, map_cur->phy_page);
		}
		map_cur = map_cur->next;
	}
	
	handler_reg* reg = (void*)(actual_rsp0 + RPOCESS_RSP0_SIZE - 16 - sizeof(handler_reg));// does not support multuple rsp0 pages
	handler_reg* reg2 = reg - 1;
	new_p->id = id_count++;
	new_p->name = "elf process (forked)";
	new_p->next = 0;
	new_p->cr3 = (uint64_t)PML4;
	new_p->rsp = (uint64_t)(process_rsp0_start + RPOCESS_RSP0_SIZE - 16- 2*sizeof(handler_reg));
	new_p->rsp_current = parent->rsp_current;
	new_p->heap_start = parent->heap_start;
	new_p->heap_break = parent->heap_break;
	new_p->on_hold = 0;
	int source_dir_len = strlen(parent->workdir);
	new_p->workdir = sf_malloc(source_dir_len+1);
	memcpy(new_p->workdir, parent->workdir, source_dir_len+1);
	// duplicate file entries
	for(int i=0; i<FD_SIZE; i++){
		open_file_descriptor* fd = parent->open_fd[i];
		if(fd->file_entry){
			fd->file_entry->open_count++;
			new_p->open_fd[i] = fd;
		}
	}
	// setup basic rsp0 content
	memset(reg, 0, sizeof(handler_reg));
	reg->cs = USER_CODE_SEGMENT_SELECTOR;
	reg->ss = USER_STACK_SEGMENT_SELECTOR;
	reg->eflags = EFLAG_INTERRUPT; // enable interrupt
	reg->ret_rsp = parent->saved_reg.ret_rsp;
	reg->ret_rip = parent->saved_reg.ret_rip;
	// setup basic rsp0 context switch content
	memset(reg2, 0, sizeof(handler_reg));
	reg2->cs = KERNEL_CODE_SEGMENT_SELECTOR;
	reg2->ss = KERNEL_STACK_SEGMENT_SELECTOR;
	reg2->eflags = EFLAG_NO_INTERRUPT; // disable interrupt
	reg2->ret_rsp = (uint64_t)(process_rsp0_start + RPOCESS_RSP0_SIZE - 16- sizeof(handler_reg));
	reg2->ret_rip = (uint64_t)AFTER_CONTEXT_SWITCH;
	// copy the registers, rbp, r9, r8, rax, rcx, rdx, rbx, rsi, rdi;
	reg->rbp = parent->saved_reg.rbp;
	reg->r9  = parent->saved_reg.r9 ;
	reg->r8  = parent->saved_reg.r8 ;
	reg->rax = 0;
	reg->rcx = parent->saved_reg.rcx;
	reg->rdx = parent->saved_reg.rdx;
	reg->rbx = parent->saved_reg.rbx;
	reg->rsi = parent->saved_reg.rsi;
	reg->rdi = parent->saved_reg.rdi;
	// attach new process to the end
	if(!first_process) first_process = new_p;
	else{
		Process* cursor = first_process;
		while(cursor->next){
			cursor = cursor->next;
		}
		cursor->next = new_p;
	}
	
	kprintf("DEBUG: thread forked, cr3: %x\n", PML4);
	return new_p;
}

// this function does not write code to the replaced process
void replace_process(Process* proc, program_section* section, uint64_t* initial_stack, uint64_t initial_stack_size){
	// switch to kernel task mode first
	
	m_map* rsp0_map = 0;
	m_map* map_cur = proc->first_map;
	while(map_cur){
		if(map_cur->type != 4) free_page_for_program(map_cur->phy_page, map_cur);
		else{
			rsp0_map = map_cur;
		}
		map_cur = map_cur->next;
	}
	assert(rsp0_map != 0, "replace_process: rsp0 map not found\n");
	
	map_cur = proc->first_map;
	while(map_cur){
		m_map* next = map_cur->next;
		if(map_cur->type != 4) sf_free(map_cur);
		map_cur = next;
	}
	
	// respawn a process on top of proc
	Process* new_p = proc;
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
	
	PML4E* pml2 = PML4 + 510; // upper pml4 entry
	pml2->P = 1;
	pml2->RW = 1;
	pml2->US = 0;
	pml2->PDPE_addr = (uint64_t)0 >> 12; // a simple direct mapping
	
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
	
	// restore saved page map
	pdp1_map->next = rsp0_map;
	void* actual_rsp0 = map_page_for_process(new_p, (uint64_t)process_rsp0_start, 1, 4, (uint64_t)rsp0_map->phy_page->base);
	rsp0_map->next = 0;
	
	// create one page for the stack now
	assert(RPOCESS_RSP0_SIZE == 4096, "ERROR: only 4096 rsp0 stack supported\n");
	void* process_initial_rsp_av = process_initial_rsp - initial_stack_size;
	for(uint64_t counter = 0; counter< initial_stack_size; counter+=4096){
		add_page_for_process(new_p, (uint64_t)process_initial_rsp - 16 - counter, 1, 2);
	}
	
	// alloc data pages
	program_section* section_cursor = section;
	uint64_t section_height_max = 0;
	while(section_cursor){
		uint64_t prog_disk = section_cursor->file_offset;
		uint64_t prog_mem = section_cursor->memory_offset;
		uint64_t to_read = 0;
		uint64_t remain = section_cursor->size;
		for(; remain != 0; 
			prog_disk += to_read, prog_mem += to_read, remain -= to_read){
			to_read = math_min(4096, 4096-(prog_mem % 4096), remain, (uint64_t)-1);
			add_page_for_process(new_p, prog_mem, 1, 2);
			// don't put data (code) yet
		}
		section_cursor = section_cursor->next;
	}
	
	handler_reg* reg = (void*)(actual_rsp0 + RPOCESS_RSP0_SIZE - 16 - sizeof(handler_reg));// does not support multuple rsp0 pages
	// new_p->id = id_count++;
	new_p->name = "executed process";
	new_p->cr3 = (uint64_t)PML4;
	new_p->rsp = (uint64_t)(process_rsp0_start + RPOCESS_RSP0_SIZE - 16- 2*sizeof(handler_reg));
	new_p->rsp_current = (uint64_t)(process_initial_rsp_av);
	new_p->heap_start = section_height_max;
	new_p->heap_break = section_height_max;
	// setup basic rsp0 content
	memset(reg, 0, sizeof(handler_reg));
	reg->cs = USER_CODE_SEGMENT_SELECTOR;
	reg->ss = USER_STACK_SEGMENT_SELECTOR;
	reg->eflags = EFLAG_INTERRUPT; // enable interrupt
	reg->ret_rsp = (uint64_t)(process_initial_rsp_av - 8);
	reg->ret_rip = section->entry_point;
	
	kprintf("DEBUG: thread replaced, cr3: %x\n", PML4);
}

int process_add_signal(uint32_t pid, uint64_t signal){
	Process* c = first_process;
	while(c){
		if(c->id == pid){
			c->sig_pending = signal;
			return 1;
		}
		c = c->next;
	}
	return 0;
}

Process* search_process(uint32_t pid){
	Process* c = first_process;
	while(c){
		if(c->id == pid){
			return c;
		}
		c = c->next;
	}
	return 0;
	
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
	
	// close all opened files
	for(int i=0; i < FD_SIZE; i++){
		if(proc->open_fd[i]){
			file_close(proc->open_fd[i]->file_entry);
		}
	}
	
	// free strings
	sf_free(proc->workdir);
	
	// now proc still exist with everything reclaimed, it is a zombie now. 
	proc->cleaned = 1;
}

int check_and_handle_rw_page_fault(Process* proc, uint64_t addr/* accessed address */){
	m_map* map_cur = proc->first_map;
	while(map_cur->next){
		m_map* curr_map = map_cur->next;
		if((curr_map->type == 2) && (curr_map->vir_addr>>12 == addr>>12)){
			if(curr_map->rw){
				void* new_page = add_page_for_process(proc, addr, 1, 2);
				// kprintf("cow split: %x, %x, %x\n", curr_map->phy_page, new_page, addr);
				memcpy(new_page, curr_map->phy_page->base, 4096);
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
			// process_cleanup(next);
			kernel_space_task_file.type = TASK_PROC_CLEANUP;
			kernel_space_task_file.param[0] = (uint64_t)next;
			kernel_space_handler_wrapper();
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
	// kprintf("DEBUG: process_schedule next: %p\n", next);
	previous_process = current_process;
	current_process = next;
	
	// if(previous_process != current_process){
		// kprintf("DEBUG: process_schedule: switch to %d\n", current_process->id);
	// }
}

void save_current_state(handler_reg volatile reg){
	if(current_process){
		current_process->rsp = reg.ret_rsp;
		current_process->rip = reg.ret_rip;
	}
}

void save_current_rsp(uint64_t rsp){
	if(current_process) current_process->rsp = rsp;
}

void save_previous_rsp(uint64_t rsp){
	if(previous_process) previous_process->rsp = rsp;
}

void save_previous_rip(uint64_t rip){
	if(previous_process) previous_process->rip = rip;
}

void save_current_cr3(uint64_t cr3){
	if(current_process){
		current_process->cr3 = cr3;
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

uint64_t get_rsp0_stack(){
	return (uint64_t)rsp0_stack+RSP0_STACK_SIZE - 16;
}

uint64_t get_kernel_cr3(){
	return (uint64_t)kernel_page_table_PML4;
}