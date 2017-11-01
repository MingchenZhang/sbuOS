

#ifndef _PHY_PAGE_H
#define _PHY_PAGE_H

typedef struct page_entry page_entry;
typedef struct page_use_record page_use_record; 

#include <sys/defs.h>
#include <sys/thread/kthread.h>

typedef struct CR3{
	uint64_t reserve1:  3;
	uint64_t PWT:       1;
	uint64_t PCD:       1;
	uint64_t reserve2:  7;
	uint64_t address:  40;
	uint64_t reserve3: 12;
}__attribute__((__packed__)) CR3;

typedef struct PML4E{
	uint64_t P:         1; // present?
	uint64_t RW:        1; // read/write?
	uint64_t US:        1; // ring0 only?
	uint64_t PWT:       1; // writeback caching policy? or writethrough caching policy?
	uint64_t PCD:       1; // not cacheable?
	uint64_t A:         1; // has been accessed?
	uint64_t IGN:       1;
	uint64_t MBZ:       2;
	uint64_t space1:    3;
	uint64_t PDPE_addr:40;
	uint64_t space2:   11;
	uint64_t NX:        1; // no execute?
}__attribute__((__packed__)) PML4E;

typedef struct PDPE{
	uint64_t P:         1;
	uint64_t RW:        1;
	uint64_t US:        1;
	uint64_t PWT:       1;
	uint64_t PCD:       1;
	uint64_t A:         1;
	uint64_t IGN:       1;
	uint64_t PS:        1;
	uint64_t MBZ:       1;
	uint64_t space1:    3;
	uint64_t PDPE_addr:40;
	uint64_t space2:   11;
	uint64_t NX:        1;
}__attribute__((__packed__)) PDPE;

typedef struct PDE{
	uint64_t P:         1;
	uint64_t RW:        1;
	uint64_t US:        1;
	uint64_t PWT:       1;
	uint64_t PCD:       1;
	uint64_t A:         1;
	uint64_t IGN:       1;
	uint64_t PS:        1;
	uint64_t IGN2:      1;
	uint64_t space1:    3;
	uint64_t PDPE_addr:40;
	uint64_t space2:   11;
	uint64_t NX:        1;
}__attribute__((__packed__)) PDE;

typedef struct PTE{
	uint64_t P:         1;
	uint64_t RW:        1;
	uint64_t US:        1;
	uint64_t PWT:       1;
	uint64_t PCD:       1;
	uint64_t A:         1;
	uint64_t D:         1;
	uint64_t PAT:       1;
	uint64_t G:         1;
	uint64_t space1:    3;
	uint64_t PDPE_addr:40;
	uint64_t space2:   11;
	uint64_t NX:        1;
}__attribute__((__packed__)) PTE;

typedef struct page_use_record{
	char type; // 0: undefined, 1:process paging(points to m_map)
	uint64_t pt;
	struct page_use_record* next;
} page_use_record;

typedef struct page_entry{
	void* base;
	char used_by; // 0: not used, 1: page entry, 2: kernel page table, 3: user process
	page_use_record* use_record;
} page_entry;

typedef struct kmalloc_usage_entry{
	void* phy_addr;
} kmalloc_usage_entry;

PDE* kernel_malloc_pd;
PDE* kernel_base_pd;

extern PML4E* kernel_page_table_PML4;

void phy_page_init(uint32_t *modulep);

void* get_phy_page(uint32_t num, char used_by);
page_entry* get_phy_page_for_program(Process* proc, m_map* map);
void dup_page_for_program(page_entry* page, m_map* map);
void free_page_for_program(page_entry* page, m_map* map);

void kernel_page_table_init();

void* kbrk(uint64_t size);

uint64_t translate_cr3(uint64_t pml4, uint64_t virtual_addr);
int set_pte_rw(uint64_t pml4, uint64_t virtual_addr, char rw);

#endif