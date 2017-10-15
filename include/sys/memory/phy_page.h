#ifndef _PHY_PAGE_H
#define _PHY_PAGE_H

#include <sys/defs.h>

void phy_page_init(uint32_t *modulep);

void* get_phy_page(uint32_t num, char used_by);

void kernel_page_table_init();

void* kbrk(uint64_t size);

#endif