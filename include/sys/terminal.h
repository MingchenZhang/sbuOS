#ifndef __TERMINAL_H
#define __TERMINAL_H

#include <sys/disk/file_system.h>
#include <sys/defs.h>

file_table_entry* terminal_file_in[2];
file_table_entry* terminal_file_out[2];

uint64_t foreground_pid;

void init_terminal();

void terminal_input_ascii(char ascii);

void update_terminal_out();

#endif