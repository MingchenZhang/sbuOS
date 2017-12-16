#include <sys/defs.h>
#include <sys/memory/kmalloc.h>
#include <sys/kprintf.h>
#include <sys/disk/file_system.h>
#include <sys/thread/kthread.h>
#include <sys/idt.h>
#include <sys/terminal.h>

#include <signal.h>

#define INPUT_BUFFER_LENGTH 256

char* terminal_buffer;
int terminal_buffer_end = 0;
file_table_entry* terminal_file_in[2]; // things enter the terminal from process
file_table_entry* terminal_file_out[2]; // things leave the terminal to process

void init_terminal(){
	terminal_buffer = sf_malloc(INPUT_BUFFER_LENGTH);
	generate_entry_pair(terminal_file_in);
	generate_entry_pair(terminal_file_out);
	foreground_pid = 0;
}

void terminal_input_ascii(char ascii){
	if(terminal_buffer_end == INPUT_BUFFER_LENGTH) return;
	terminal_buffer[terminal_buffer_end++] = ascii;
	if(ascii == '\n'){
		terminal_buffer[terminal_buffer_end] = 0;
		// kprintf("terminal receives: %s", terminal_buffer);
		file_write(terminal_file_out[1], 0, (uint8_t*)terminal_buffer, terminal_buffer_end);
		terminal_buffer_end = 0;
	}else if(ascii == 0x8){ // back space
		if(terminal_buffer_end > 1)terminal_buffer_end-=2;
		else terminal_buffer_end--;
	}else if(ascii == 24){ // ctr-c
		terminal_buffer_end--;
		if(foreground_pid) process_add_signal(foreground_pid, SIGINT);
	}
	print_terminal_input_line(terminal_buffer, terminal_buffer_end);
}

void update_terminal_out(){
	uint8_t buffer[512];
	int readed = file_read(terminal_file_in[0], 0, buffer, 511);
	if(readed != 0){
		buffer[readed] = 0;
		kprintf((char*)buffer);
	}
}