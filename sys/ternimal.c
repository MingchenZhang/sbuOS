#include <sys/defs.h>
#include <sys/memory/kmalloc.h>
#include <sys/kprintf.h>

#define INPUT_BUFFER_LENGTH 256

char* terminal_buffer;
int terminal_buffer_end = 0;

void init_terminal(){
	terminal_buffer = sf_malloc(INPUT_BUFFER_LENGTH);
}

void terminal_input_ascii(char ascii){
	if(terminal_buffer_end == INPUT_BUFFER_LENGTH) return;
	terminal_buffer[terminal_buffer_end++] = ascii;
	if(ascii == '\n'){
		terminal_buffer[terminal_buffer_end] = 0;
		kprintf("terminal receives: %s", terminal_buffer);
		terminal_buffer_end = 0;
	}
	if(ascii == 0x8){
		if(terminal_buffer_end > 1)terminal_buffer_end-=2;
		else terminal_buffer_end--;
	}
	print_terminal_input_line(terminal_buffer, terminal_buffer_end);
}