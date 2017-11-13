#include <sys/thread/kthread.h>
#include <sys/memory/kmalloc.h>
#include <sys/defs.h>
#include <sys/misc.h>
#include <sys/timer.h>

waiter* first_waiter = 0;

void register_to_be_waken(Process* proc, uint64_t ticks_to_wake){
	waiter* cursor = first_waiter;
	if(!cursor){
		first_waiter = sf_calloc(sizeof(waiter), 1);
		memset(first_waiter, 0, sizeof(waiter));
		first_waiter->proc = proc;
		first_waiter->ticks_to_wake = ticks_to_wake;
		// proc->on_hold = 1;
		return;
	}
	while(cursor->next){
		cursor = cursor->next;
	}
	waiter* new_waiter = sf_calloc(sizeof(waiter), 1);
	memset(new_waiter, 0, sizeof(waiter));
	new_waiter->proc = proc;
	new_waiter->ticks_to_wake = ticks_to_wake;
	cursor->next = new_waiter;
	proc->on_hold = 1;
}

void tick_timer_update(){
	waiter* cursor = first_waiter;
	if(!cursor){
		return;
	}
	if(cursor->ticks_to_wake-- == 0){
		cursor->proc->on_hold = 0;
		first_waiter = cursor->next;
		sf_free(cursor);
		if(!first_waiter) return;
		cursor = first_waiter;
	}// TODO: not yet fully exhaust the list
	while(cursor->next){
		if(cursor->next->ticks_to_wake-- == 0){
			cursor->next->proc->on_hold = 0;
			waiter* to_be_removed = cursor->next;
			cursor->next = to_be_removed->next;
			sf_free(to_be_removed);
		}else{
			cursor = cursor->next;
		}
	}
}