#include <sys/memory/phy_page.h>
#include <sys/memory/kmalloc.h>
#include <sys/thread/kthread.h>
#include <sys/misc.h>
#include <sys/kprintf.h>




struct page_entry* page_entry_base;
struct page_entry* page_entry_end;

PML4E* kernel_page_table_PML4;

#define KBRK_BASE_ADDRESS (void*)(0xFFFFFFFF80000000)

#define MEMORY_CAP 0x6400000//+0xF000000000000000

struct page_entry* find_free_page_entry(){
	for(struct page_entry* cursor = page_entry_base; cursor < page_entry_end; cursor++){
		if(cursor->used_by == 0){
			return cursor;
		}
	}
	return 0;
}

void phy_page_init(uint32_t *modulep){
	
	struct smap_t {
	uint64_t base, length;
	uint32_t type;
	}__attribute__((packed)) *smap;
	
	uint64_t total_page_count = 0;
	
	while(modulep[0] != 0x9001) modulep += modulep[1]+2;
	for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
		if (smap->type == 1 /* memory */ && smap->length != 0) {
			kprintf("Available Physical Memory [%p-%p]\n", smap->base, smap->base + smap->length);
			total_page_count += smap->length / 4096;
		}
	}
	
	uint64_t page_table_size = total_page_count * sizeof(struct page_entry);
	uint64_t total_space = 0;
	
	int count = 0;
	for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
		uint64_t smap_base = smap->base;
		uint64_t smap_length = smap->length;
		if (smap->type == 1 /* memory */ && smap_length != 0) {
			if(count == 1){ // put memory map at second zone
				if(smap_base != 0x100000) {
					kprintf("PANIC: second smap is not at 0x100000\n");
					panic_halt();
				}
				// shift 0x400000 for kernal space
				smap_base = 0x400000;
				smap_length -= 0x300000;
				// if(smap_base % 4096 != 0) {
					// kprintf("PANIC: second memory zone not aligned\n");
					// panic_halt();
				// }
				if(page_table_size > smap_length) {
					kprintf("PANIC: not enough space to place page entry\n");
					panic_halt();
				}
				// pre allocate first page
				page_entry_base = (void*)smap_base;
				page_entry_base->base = (void*)smap_base;
				page_entry_base->used_by = 1;
				page_entry_end = page_entry_base + 1;
				uint8_t* zone_cursor = (void*)(smap_base + 4096);
				uint8_t* zone_end = (void*)(smap_base + smap_length);
				for(;zone_cursor < zone_end; zone_cursor += 4096, total_space += 4096){
					// cap the memory used
					if(total_space > MEMORY_CAP) break;
					if((uint64_t)page_entry_end % 4096 == 0){
						// more page is needed for page entry
						struct page_entry* e = find_free_page_entry();
						e->used_by = 1;
					}
					page_entry_end->base = (void*)zone_cursor;
					page_entry_end->used_by = 0;
					page_entry_end++;
				}
			} else { // deal with other zones
				if(smap_base % 4096 != 0) {
					// kprintf("PANIC: memory zone not aligned\n");
					// panic_halt();
				}else{
					uint8_t* zone_cursor = (void*)smap_base;
					uint8_t* zone_end = (void*)(smap_base + smap_length);
					for(;zone_cursor < zone_end; zone_cursor += 4096, total_space += 4096){
						// cap the memory used
						if(total_space > MEMORY_CAP) break;
						if((uint64_t)page_entry_end % 4096 == 0){
							// more page is needed for page entry
							struct page_entry* e = find_free_page_entry();
							e->used_by = 1;
						}
						page_entry_end->base = (void*)zone_cursor;
						page_entry_end->used_by = 0;
						page_entry_end++;
					}
				}
				
			}
			count++;
		}
	}
	
	kprintf("memory pages initialized\n");
}

void* get_phy_page(uint32_t num, char used_by){
	if(num > 1) {
		kprintf("PANIC: get_phy_page not support multiple pages yet\n");
		panic_halt();
	}
	page_entry* new_page = find_free_page_entry();
	new_page->used_by = used_by;
	// kprintf("get_phy_page: allocated one at: %p\n", new_page->base);
	return new_page->base;
}

page_entry* get_phy_page_for_program(Process* proc, m_map* map){
	page_entry* new_page = find_free_page_entry();
	new_page->used_by = 3;
	new_page->use_record = sf_calloc(sizeof(page_use_record), 1);
	memset(new_page->use_record, 0, sizeof(page_use_record));
	new_page->use_record->type = 1;
	new_page->use_record->pt = (uint64_t)map;
	return new_page;
}

void dup_page_for_program(page_entry* page, m_map* map){
	assert(page->used_by == 3, "dup_page_for_program: dup_page_for_program called on non program page\n");
	page_use_record* new_record = sf_calloc(sizeof(page_use_record), 1);
	memset(new_record, 0, sizeof(page_use_record));
	page->use_record->type = 1;
	page_use_record* cursor = page->use_record;
	assert(cursor != 0, "dup_page_for_program: use_record is empty\n");
	while(cursor->next){
		cursor = cursor->next;
	}
	cursor->next = new_record;
	new_record->type = 1;
	new_record->pt = (uint64_t) map;
}

void free_page_for_program(page_entry* page, m_map* map){
	if(!(page->used_by == 3)){
		kprintf("free_page_for_program: page is not a user program page (type:%d)\n", page->used_by);
		while(1);
	}
	page_use_record* cursor = page->use_record;
	assert(cursor != 0, "free_page_for_program: use_record is empty\n");
	if((m_map*)cursor->pt == map){
		// this the last free for this page, now actually free it
		sf_free(cursor);
		page->used_by = 0;
		return;
	}
	// else delete from linked list
	char found = 0;
	while(cursor->next){
		if((m_map*)cursor->next->pt == map){
			page_use_record* old_record = cursor->next;
			cursor->next = old_record->next;
			sf_free(old_record);
			found = 1;
		}else{
			cursor = cursor->next;
		}
	}
	assert(found, "free_page_for_program unable to found page\n");
}

void kernel_page_table_init(){
	PML4E* PML4 = get_phy_page(1, 2);
	PDPE* PDP1 = get_phy_page(1, 2);
	PDPE* PDP2 = get_phy_page(1, 2);
	PDE* PD = get_phy_page(1, 2);
	kernel_malloc_pd = PD;
	kernel_base_pd = get_phy_page(1, 2);
	
	// initialize kernel base PD page table
	memset(kernel_base_pd, 0, 4096);
	for(int i=0; i<512; i++){
		PDE* _pd = kernel_base_pd + i;
		_pd->P = 1;
		_pd->RW = 1;
		_pd->US = 1;
		_pd->PS = 1;
		_pd->PDPE_addr = (uint64_t)(0x200000 * i) >> 12;
	}
	
	memset(PML4, 0, 4096);
	memset(PDP1, 0, 4096);
	memset(PDP2, 0, 4096);
	memset(PD, 0, 4096);
	
	PML4E* pml1 = PML4 + 511; // upper pml4 entry
	pml1->P = 1;
	pml1->RW = 1;
	pml1->US = 0;
	pml1->PDPE_addr = (uint64_t)PDP1 >> 12;
	
	PML4E* pml3 = PML4 + 509; // upper pml4 entry
	pml3->P = 1;
	pml3->RW = 1;
	pml3->US = 0;
	pml3->PDPE_addr = (uint64_t)0 >> 12; // direct simple map
	
	PML4E* pml2 = PML4 + 0; // lower pml4 entry
	pml2->P = 1;
	pml2->RW = 1;
	pml2->US = 0;
	pml2->PDPE_addr = (uint64_t)PDP2 >> 12;
	
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
	pdp2->PDPE_addr = (uint64_t)PD >> 12;
	
	PDPE* pdp4 = PDP2 + 0; // lower pdp entry // TODO: may create more for more than 1GB access
	pdp4->P = 1;
	pdp4->RW = 1;
	pdp4->US = 0;
	pdp4->PS = 0;
	pdp4->PDPE_addr = (uint64_t)kernel_base_pd >> 12;
	
	PDE* pd0 = PD + 0; // kmalloc pd entry
	pd0->P = 1;
	pd0->RW = 1;
	pd0->US = 0;
	pd0->PS = 1;
	pd0->PDPE_addr = 0; // points to the start of the memory
	
	PDE* pd1 = PD + 1; // kmalloc pd entry 2nd
	pd1->P = 1;
	pd1->RW = 1;
	pd1->US = 0;
	pd1->PS = 1;
	pd1->PDPE_addr = 0x200000 >> 12; // points to next region
	
	kernel_page_table_PML4 = PML4;
	
	process_direct_mapping_addr = (void*)0xFFFFFF0000000000; // direct simple map
	
	__asm__ volatile("movq %0, %%cr3"::"r"((uint64_t)PML4));
	kprintf("kernel page table initialized\n");
}

void change_kernel_rsp0(uint64_t change_to){
	PTE* pt1 = (PTE*)0xFFFFFFFF7FFFF000 + 510;
	kprintf("change_kernel_rsp0: get address, try to read: %x\n", *(uint64_t*)pt1);
	kprintf("change_kernel_rsp0: read done, try to write: %x\n", change_to);
	pt1->PDPE_addr = change_to >> 12;
}

static void move_kbrk(PDE* pd, PTE* pt){
	if(pt){ // just add a PT entry
		void* new_page = get_phy_page(1, 2);
		pt->P = 1;
		pt->RW = 1;
		pt->US = 0;
		pt->PDPE_addr = (uint64_t)new_page>>12;
	}else{ // need to also add pd entry
		void* new_page = get_phy_page(1, 2);
		void* new_page_pt = get_phy_page(1, 2);
		pd->P = 1;
		pd->RW = 1;
		pd->US = 0;
		pd->PS = 0;
		pd->PDPE_addr = (uint64_t)new_page_pt>>12;
		PTE* pt = new_page_pt;
		pt->P = 1;
		pt->RW = 1;
		pt->US = 0;
		pt->PDPE_addr = (uint64_t)new_page>>12;
	}
}

static void* _kbrk(uint64_t size){
	// kprintf("DEBUG: _kbrk %d\n", size);
	if(size > 4096){
		kprintf("PANIC: _kbrk not yet support multiple pages\n");
		panic_halt();
	}
	PDE* pd_base = (PDE*)(uint64_t)(((PDPE*)(uint64_t)(kernel_page_table_PML4[511].PDPE_addr<<12))[510].PDPE_addr<<12);
	for(int i=2; i<512; i++){
		if(!pd_base[i].P){
			i--;
			if(i == 1){
				if(size > 0) move_kbrk(pd_base+i+1, 0);
				return (void*)(uint64_t)(KBRK_BASE_ADDRESS + 0x400000); // when no space has been allocated
			}else{
				PTE* pt_base = (PTE*)(uint64_t)(pd_base[i].PDPE_addr<<12);
				for(uint32_t j=0; j<512; j++){ // check which pt entry is absent to tell where is the break point
					if(!pt_base[j].P){
						if(size > 0) move_kbrk(pd_base+i, pt_base+j);
						return (void*)(uint64_t)(KBRK_BASE_ADDRESS + i * 0x200000 + j * 0x1000);
					}
				}
				if(size > 0) move_kbrk(pd_base+i+1, 0);
				return (void*)(uint64_t)(KBRK_BASE_ADDRESS + (i+1) * 0x200000);
			}
		}
	}
	kprintf("PANIC: kbrk not yet support space beyand 1GB\n");
	panic_halt();
	return 0;
}

void* kbrk(uint64_t size) {
	if(size % 4096 != 0){
		kprintf("PANIC: kbrk could not allocate non 4096 aligned space\n");
		panic_halt();
	}
	if(size == 0) return _kbrk(0);
	void* ret = _kbrk(4096);
	for(size-=4096;size>0; size -= 4096){
		_kbrk(4096);
	}
	return ret;
}

uint64_t translate_cr3(uint64_t pml4, uint64_t virtual_addr){
	// TODO: add early terminated page table support
	register uint64_t pdp = (((uint64_t*)(pml4 & 0xFFFFFFFFFF000))[(virtual_addr>>39) & 0b111111111]);
	if(!(pdp & 0x1)) return 0;
	register uint64_t pd = (((uint64_t*)(pdp & 0xFFFFFFFFFF000))[(virtual_addr>>30) & 0b111111111]);
	if(!(pd & 0x1)) return 0;
	register uint64_t pt = (((uint64_t*)(pd & 0xFFFFFFFFFF000))[(virtual_addr>>21) & 0b111111111]);
	if(!(pt & 0x1)) return 0;
	register uint64_t pte = (((uint64_t*)(pt & 0xFFFFFFFFFF000))[(virtual_addr>>12) & 0b111111111]);
	if(!(pte & 0x1)) return 0;
	return (pte & 0xFFFFFFFFFF000) + (virtual_addr & 0xFFF);
}

int set_pte_rw(uint64_t pml4, uint64_t virtual_addr, char rw){
	register uint64_t pdp = (((uint64_t*)(pml4 & 0xFFFFFFFFFF000))[(virtual_addr>>39) & 0b111111111]);
	if(!(pdp & 0x1)) return 0;
	register uint64_t pd = (((uint64_t*)(pdp & 0xFFFFFFFFFF000))[(virtual_addr>>30) & 0b111111111]);
	if(!(pd & 0x1)) return 0;
	register uint64_t pt = (((uint64_t*)(pd & 0xFFFFFFFFFF000))[(virtual_addr>>21) & 0b111111111]);
	if(!(pt & 0x1)) return 0;
	register uint64_t pte = (((uint64_t*)(pt & 0xFFFFFFFFFF000))[(virtual_addr>>12) & 0b111111111]);
	if(!(pte & 0x1)) return 0;
	if(rw) (((uint64_t*)(pt & 0xFFFFFFFFFF000))[(virtual_addr>>12) & 0b111111111]) &= 
		0xFFFFFFFFFFFFFFFD;
	else (((uint64_t*)(pt & 0xFFFFFFFFFF000))[(virtual_addr>>12) & 0b111111111]) |= 
		0x2;
	return 1;
}