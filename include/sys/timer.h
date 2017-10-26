#include <sys/defs.h>

#ifndef _TIMER_H
#define _TIMER_H

typedef struct waiter{
	Process* proc;
	uint64_t ticks_to_wake;
	struct waiter* next;
} waiter;

void register_to_be_waken(Process* proc, uint64_t ticks_to_wake);

void tick_timer_update();

#endif