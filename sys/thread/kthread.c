#include <sys/kprintf.h>
#include <sys/defs.h>
#include <sys/memory/phy_page.h>
#include <sys/memory/kmalloc.h>
#include <sys/misc.h>
#include <sys/thread/kthread.h>

Process* first_process = 0;
Process* current_process = 0;
Process* previous_process = 0;
uint32_t id_count = 0;

uint64_t create_process_page_table(Process_init proc){
	PML4E* PML4 = get_phy_page(1, 3);
	PDPE* PDP1 = get_phy_page(1, 3);
	PDPE* PDP3 = get_phy_page(1, 3);
	PDE* PD = get_phy_page(1, 3);
	PDE* PD2 = get_phy_page(1, 3);
	PTE* PT = get_phy_page(1, 3);
	uint64_t stack_page = (uint64_t)get_phy_page(1, 3);
	memset(PML4, 0, 4096);
	memset(PDP1, 0, 4096);
	memset(PDP3, 0, 4096);
	memset(PD, 0, 4096);
	memset(PD2, 0, 4096);
	memset(PT, 0, 4096);
	
	PML4E* pml1 = PML4 + 511; // upper pml4 entry
	pml1->P = 1;
	pml1->RW = 1;
	pml1->US = 0;
	pml1->PDPE_addr = (uint64_t)PDP1 >> 12;
	
	PML4E* pml2 = PML4 + 0; // lower pml4 entry
	pml2->P = 1;
	pml2->RW = 1;
	pml2->US = 0;
	pml2->PDPE_addr = (uint64_t)PDP3 >> 12;
	
	PDPE* pdp1 = PDP1 + 511; // upper pdp entry
	pdp1->P = 1;
	pdp1->RW = 1;
	pdp1->US = 1;
	pdp1->PS = 1;
	pdp1->PDPE_addr = 0; // points to the start of the memory
	
	PDPE* pdp2 = PDP1 + 510; // kmalloc pdp entry
	pdp2->P = 1;
	pdp2->RW = 1;
	pdp2->US = 1;
	pdp2->PS = 0;
	pdp2->PDPE_addr = (uint64_t)kernel_malloc_pd >> 12; // points to the start of the memory
	
	PDPE* pdp3 = PDP1 + 509; // stack pdp entry
	pdp3->P = 1;
	pdp3->RW = 1;
	pdp3->US = 0;
	pdp3->PDPE_addr = (uint64_t)PD >> 12;
	
	PDPE* pdp4 = PDP3 + 0; // lower pdp entry
	pdp4->P = 1;
	pdp4->RW = 1;
	pdp4->US = 0;
	pdp4->PDPE_addr = (uint64_t)PD2 >> 12;
	
	PDE* pd0 = PD + 511; // stack pd entry
	pd0->P = 1;
	pd0->RW = 1;
	pd0->US = 0;
	pd0->PS = 0;
	pd0->PDPE_addr = (uint64_t)PT >> 12;
	
	PTE* pt0 = PT + 511; // one page test stack page
	pt0->P = 1;
	pt0->RW = 1;
	pt0->US = 0;
	pt0->PDPE_addr = (uint64_t)stack_page >> 12;
	
	uint64_t ins_size = proc.ins_size;
	uint64_t ins_start = proc.ins_start;
	for(int i=1; i<512; i++){
		PTE* PT_data = get_phy_page(1, 3);
		memset(PT_data, 0, 4096);
		PDE* pd1 = PD2 + i; // data pd entry
		pd1->P = 1;
		pd1->RW = 1;
		pd1->US = 0;
		pd1->PS = 0;
		pd1->PDPE_addr = (uint64_t)PT_data >> 12;
		for(int j = 0; ins_size>0 && j<512; ins_size-=4096, ins_start+=4096, j++){
			uint64_t new_data_page = (uint64_t)get_phy_page(1, 3); // TODO: consider when new_data_page > 1G
			PTE* pt0 = PT_data + j; // one page test stack page
			pt0->P = 1;
			pt0->RW = 1;
			pt0->US = 0;
			pt0->PDPE_addr = (uint64_t)new_data_page >> 12;
			if(ins_size>4096){
				memcpy((void*)new_data_page, (void*)ins_start, 4096);
			}else{
				memcpy((void*)new_data_page, (void*)ins_start, ins_size);
				ins_size = 0;
				break;
			}
		}
		if(ins_size == 0) break;
	}
	
	return (uint64_t)PML4;
}

void test_spawn_process(void* code_base, uint64_t code_size){
	Process_init proc = {
		(uint64_t)code_base,
		(uint64_t)code_size
	};
	kprintf("DEBUG: start generating page table\n");
	uint64_t cr3 = create_process_page_table(proc);
	
	uint64_t* stack_start = (uint64_t*)(translate_cr3(cr3, 0xFFFFFFFF7FFFFFF0));
	*(stack_start - 1) = 0x10;
	*(stack_start - 2) = (uint64_t)0xFFFFFFFF7FFFFFF0; // where stack starts
	*(stack_start - 3) = 0x200216;  // TODO: understand EFLAGS
	*(stack_start - 4) = 0x8;
	*(stack_start - 5) = (uint64_t)(uint64_t*)0x200000; // where ip starts
	*(stack_start - 6) = 0;
	*(stack_start - 7) = 0x80;
	Process* new_p = sf_malloc(sizeof(Process));
	new_p->id = id_count++;
	new_p->name = "test process";
	new_p->next = 0;
	new_p->cr3 = cr3;
	new_p->rsp = (uint64_t)(0xFFFFFFFF7FFFFFF0 - 8*16);
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

void process_schedule(){
	Process* next;
	if(!current_process){
		next = first_process;
	}else{
		if(current_process->next) next = current_process->next;
		else next = first_process;
	}
	if(!next){
		kprintf("PANIC: no process to schedule\n");
		panic_halt();
	}
	// kprintf("process_schedule next: %p\n", next);
	previous_process = current_process;
	current_process = next;
}

void save_previous_rsp(uint64_t rsp){
	if(previous_process) previous_process->rsp = rsp;
}

uint64_t load_current_rsp(){
	return current_process->rsp;
}

uint64_t load_current_cr3(){
	return current_process->cr3;
}