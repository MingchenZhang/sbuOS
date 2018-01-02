
#include <sys/disk/disk_driver.h>

disk_task dummy_task;

page_entry* read_disk_block_req(uint8_t disk_i, Process* proc, uint64_t LBA){
	page_entry* w_to = find_free_page_entry();
	w_to->used_by = 2;
	disk_task* new_task = sf_calloc(sizeof(disk_task), 1);
	op_info op = ahci_read_task(disk_i, LBA, w_to->base);
	if(op.result == -1){
		// no available command slot, wait for it to cleanup
		new_task->diskID = disk_i;
		new_task->slot = 0;
		new_task->is_write = 0;
		new_task->issued = 0;
		new_task->LBA = LBA;
		new_task->rw_to = w_to;
		new_task->proc = proc;
		new_task->next = 0;
		// insert this task
		disk_task* c = &dummy_task;
		while(c->next){
			c = c->next;
		}
		c->next = new_task;
		return w_to;
	}
	if(!op.result) {
		w_to->used_by = 0; // free the page
		return 0;
	}
	new_task->diskID = disk_i;
	new_task->slot = op.slot;
	new_task->is_write = 0;
	new_task->issued = 1;
	new_task->rw_to = w_to;
	new_task->proc = proc;
	new_task->next = 0;
	// insert this task
	disk_task* c = &dummy_task;
	while(c->next){
		c = c->next;
	}
	c->next = new_task;
	return w_to;
}

int write_disk_block_req(int disk_i, Process* proc, uint64_t LBA, page_entry* page){
	op_info op = ahci_write_task(disk_i, LBA, page->base);
	if(op.result == -1){
		// no available command slot, wait for it to cleanup
		disk_task* new_task = sf_calloc(sizeof(disk_task), 1);
		new_task->diskID = disk_i;
		new_task->slot = 0;
		new_task->is_write = 1;
		new_task->issued = 0;
		new_task->LBA = LBA;
		new_task->rw_to = page;
		new_task->proc = proc;
		new_task->next = 0;
		// insert this task
		disk_task* c = &dummy_task;
		while(c->next){
			c = c->next;
		}
		c->next = new_task;
		return 1;
	}
	// kprintf("1:%d\n", op.result);
	if(!op.result) {
	// kprintf("2:%d\n", op.result);
		return 0;
	}
	// kprintf("3:%d\n", op.result);
	disk_task* new_task = sf_calloc(sizeof(disk_task), 1);
	new_task->diskID = disk_i;
	new_task->slot = op.slot;
	new_task->is_write = 1;
	new_task->issued = 1;
	new_task->rw_to = page;
	new_task->proc = proc;
	new_task->next = 0;
	// insert this task
	disk_task* c = &dummy_task;
	while(c->next){
		c = c->next;
	}
	c->next = new_task;
	return 1;
}

static inline int check_task(disk_task* task){
	if(task->issued == 0){
		if(!task->is_write){
			op_info op = ahci_read_task(task->diskID, task->LBA, task->rw_to->base);
			if(op.result == 1){
				task->issued = 1;
				task->slot = op.slot;
			}
			assert(op.result != 0, "check_task: scheduled disk task failed\n");
			return 0;
		}else{
			op_info op = ahci_write_task(task->diskID, task->LBA, task->rw_to->base);
			if(op.result == 1){
				task->issued = 1;
				task->slot = op.slot;
			}
			assert(op.result != 0, "check_task: scheduled disk task failed\n");
			return 0;
		}
	}
	int done = check_ahci_port(task->diskID, task->slot);
	if(done && !task->proc->terminated){
		task->proc->on_hold = 0;
	}
	return done;
}

void check_disk_task(){
	disk_task* c = &dummy_task;
	while(c->next){
		if(check_task(c->next)){
			// task is done
			disk_task* t = c->next;
			c->next = t->next;
			sf_free(t);
		}else{
			c = c->next;
		}
	}
}
