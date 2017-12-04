#ifndef _DISK_DRIVER_H
#define _DISK_DRIVER_H

#include <sys/defs.h>
#include <sys/thread/kthread.h>
#include <sys/kprintf.h>
#include <sys/memory/phy_page.h>
#include <sys/memory/kmalloc.h>
#include <sys/ahci.h>
#include <sys/misc.h>

typedef struct disk_task {
	uint8_t diskID;
	uint32_t slot;
	uint64_t LBA;
	uint8_t is_write: 1;
	uint8_t issued: 1;
	page_entry* rw_to;
	Process* proc;
	struct disk_task* next;
} disk_task;

page_entry* read_disk_block_req(uint8_t disk_i, Process* proc, uint64_t LBA);

int write_disk_block_req(int disk_i, Process* proc, uint64_t LBA, page_entry* page);

void check_disk_task();

#endif