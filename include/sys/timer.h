#include <sys/defs.h>

#ifndef _TIMER_H
#define _TIMER_H

typedef struct waiter{
	Process* proc;
	uint64_t ticks_to_wake;
	uint64_t is_alarm:1;
	struct waiter* next;
} waiter;

void register_to_be_waken(Process* proc, uint64_t ticks_to_wake);

void register_to_trigger_alarm(Process* proc, uint64_t ticks_to_wake);

void tick_timer_update();

#endif