#include <sys/defs.h>
#include <sys/idt.h>
#include <sys/kprintf.h>
#include <sys/misc.h>
#include <sys/config.h>
#include <sys/keyboard.h>
#include <sys/thread/kthread.h>
#include <sys/timer.h>
#include <sys/config.h>
#include <sys/memory/phy_page.h>
#include <sys/terminal.h>
#include <sys/memory/kmalloc.h>
#include <sys/disk/disk_driver.h>
#include <sys/disk/file_system.h>
#include <sys/terminal.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <errno.h>
#include <dirent.h>

#define IRQ0 32
#define IRQ1 33

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr32(); // IRQ0: used for PIT timer
extern void isr33(); // IRQ1: used for keyboard interrupt
extern void isr128(); // used for syscall
extern void isr129(); // used for context switch

extern void lidt(void* pointer);

struct idt_entry_struct
{
   uint16_t offset_1; // offset bits 0..15
   uint16_t selector; // a code segment selector in GDT or LDT
   uint8_t ist;       // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   uint8_t type_attr; // type and attributes
   uint16_t offset_2; // offset bits 16..31
   uint32_t offset_3; // offset bits 32..63
   uint32_t zero;     // reserved
} __attribute__((packed));

struct idt_ptr_struct
{
   uint16_t limit;
   uint64_t base;                // address points to first entry of idt_table
} __attribute__((packed));

struct idt_entry_struct idt_table[256];
struct idt_ptr_struct idt_ptr;

static void idt_set_entry(uint8_t num, uint64_t base, uint16_t selector, uint8_t type_attr) {
   idt_table[num].offset_1 = base & 0xFFFF;
   idt_table[num].offset_2 = (base >> 16) & 0xFFFF;
   idt_table[num].offset_3 = (base >> 32) & 0xFFFFFFFF;
   idt_table[num].selector = selector & 0xFFFF;
   idt_table[num].ist = 0;
   idt_table[num].zero = 0;
   idt_table[num].type_attr = type_attr;
}

void init_pic() { // recheck
	// restart PIC1
	asm_outb(0x20, 0x11);
	// restart PIC2
	asm_outb(0xA0, 0x11);
	// set PIC1 offset to 32
	asm_outb(0x21, 0x20);
	// set PIC2 offset to 40
	asm_outb(0xA1, 0x28);
	// setup cascading
	asm_outb(0x21, 0x04);
	asm_outb(0xA1, 0x02);
	// finishing
	asm_outb(0x21, 0x01);
	asm_outb(0xA1, 0x01);
	// outb(0x21, 0x0);
	// outb(0xA1, 0x0);
}

void init_idt(){
	idt_ptr.limit = sizeof(struct idt_entry_struct)*256 -1;
	idt_ptr.base = (uint64_t)&idt_table;
	// reset table
	for(uint8_t* i=(uint8_t*)idt_table; i< ((uint8_t*)idt_table)+sizeof(idt_table); i++) *i = (uint8_t)0;
	// setup idt_table
	idt_set_entry( 0, (uint64_t)isr0 , 0x08, 0x8E);
	idt_set_entry( 1, (uint64_t)isr1 , 0x08, 0x8E);
	idt_set_entry( 2, (uint64_t)isr2 , 0x08, 0x8E);
	idt_set_entry( 3, (uint64_t)isr3 , 0x08, 0x8E);
	idt_set_entry( 4, (uint64_t)isr4 , 0x08, 0x8E);
	idt_set_entry( 5, (uint64_t)isr5 , 0x08, 0x8E);
	idt_set_entry( 6, (uint64_t)isr6 , 0x08, 0x8E);
	idt_set_entry( 7, (uint64_t)isr7 , 0x08, 0x8E);
	idt_set_entry( 8, (uint64_t)isr8 , 0x08, 0x8E);
	idt_set_entry( 9, (uint64_t)isr9 , 0x08, 0x8E);
	idt_set_entry( 10, (uint64_t)isr10 , 0x08, 0x8E);
	idt_set_entry( 11, (uint64_t)isr11 , 0x08, 0x8E);
	idt_set_entry( 12, (uint64_t)isr12 , 0x08, 0x8E);
	idt_set_entry( 13, (uint64_t)isr13 , 0x08, 0x8E);
	idt_set_entry( 14, (uint64_t)isr14 , 0x08, 0x8E);
	idt_set_entry( 15, (uint64_t)isr15 , 0x08, 0x8E);
	idt_set_entry( 16, (uint64_t)isr16 , 0x08, 0x8E);
	idt_set_entry( 17, (uint64_t)isr17 , 0x08, 0x8E);
	idt_set_entry( 18, (uint64_t)isr18 , 0x08, 0x8E);
	idt_set_entry( 19, (uint64_t)isr19 , 0x08, 0x8E);
	idt_set_entry( 20, (uint64_t)isr20 , 0x08, 0x8E);
	idt_set_entry( 21, (uint64_t)isr21 , 0x08, 0x8E);
	idt_set_entry( 22, (uint64_t)isr22 , 0x08, 0x8E);
	idt_set_entry( 23, (uint64_t)isr23 , 0x08, 0x8E);
	idt_set_entry( 24, (uint64_t)isr24 , 0x08, 0x8E);
	idt_set_entry( 25, (uint64_t)isr25 , 0x08, 0x8E);
	idt_set_entry( 26, (uint64_t)isr26 , 0x08, 0x8E);
	idt_set_entry( 27, (uint64_t)isr27 , 0x08, 0x8E);
	idt_set_entry( 28, (uint64_t)isr28 , 0x08, 0x8E);
	idt_set_entry( 29, (uint64_t)isr29 , 0x08, 0x8E);
	idt_set_entry( 30, (uint64_t)isr30 , 0x08, 0x8E);
	idt_set_entry( 31, (uint64_t)isr31 , 0x08, 0x8E);
	idt_set_entry( 32, (uint64_t)isr32 , 0x08, 0x8E);
	idt_set_entry( 33, (uint64_t)isr33 , 0x08, 0x8E);
	idt_set_entry( 128, (uint64_t)isr128 , 0x08, 0xEE);
	idt_set_entry( 129, (uint64_t)isr129 , 0x08, 0x8E);
	// apply the changes
	lidt(&idt_ptr);
}

static inline void test_tick_handle(){
	tick_timer_update();
	if(pic_tick_count%PIT_FREQUENCY == 0){
		update_topright_display();
	}
}

static void handle_sig_pending(volatile handler_reg* reg){
	if(current_process->sig_pending && current_process->sig_saved_reg.int_num == 0 && current_process->sig_handler){
		if(current_process->sig_pending == SIGKILL){ // kill
			current_process->on_hold = 1;
			current_process->terminated = 1; // label to be cleaned
		}else{ // normal signal
			memcpy(&current_process->sig_saved_reg, (handler_reg*)reg, sizeof(handler_reg));
			reg->ret_rip = current_process->sig_handler;
			reg->rdi = current_process->sig_pending;
			current_process->sig_pending = 0;
		}
	}
}

void trigger_ctx_switch(){
	__asm__ volatile ("int $0x81;");
}

int64_t isr_handler(handler_reg volatile reg){
	int64_t ret = 0;
	// reset PIC first
	if(reg.int_num >= 32 && reg.int_num < 48){ // a PIC interrupt
		if (reg.int_num >= 40){ // it came from PIC
			asm_outb(0xA0, 0x20); // Send EOI to slave.
		}
		asm_outb(0x20, 0x20);// Send EOI to master
	}
	
	if(reg.int_num == IRQ0){ // timer interrupt (from PIT)
		kernel_space_task_file.type = TASK_TIMER_TICK;
		kernel_space_handler_wrapper();
		// also update terminal out
		update_terminal_out();
		handle_sig_pending(&reg);
		ret = 1; // will trigger context switch
	}else if(reg.int_num == IRQ1){ // keyboard interrupt
		kernel_space_task_file.type = TASK_KEYBOARD_HANDLE;
		kernel_space_handler_wrapper();
	}else if(reg.int_num == 0x80){
		if(!current_process){
			ret = 1;
			goto finally;
		}
		// if(reg.rax != 103 && reg.rax != 104){
			// uint64_t program_cr3;
			// __asm__ volatile("movq %%cr3, %0":"=r"(program_cr3):);
			// kprintf("(%x:%d)", program_cr3, reg.rax);
		// }
		// syscall return must be written back to reg.rax
		if(reg.rax == 12){
			// int64_t sys_ioctl(int fd, uint64_t op, uint64_t arg);
			// uint64_t fd = reg.rdi;
			uint64_t op = reg.rsi;
			uint64_t arg = reg.rdx;
			if(op == TIOCSPGRP){
				// TODO: check fd points to
				foreground_pid = arg;
				reg.rax = 0;
			}else{
				kprintf("warning: non supported ioctl operation\n");
				reg.rax = (uint64_t)-1;
			}
		}else if(reg.rax == 100){
			// int64_t sys_signal(pid_t pid, uint64_t sig);
			uint32_t pid = (uint32_t)reg.rdi;
			uint64_t sig_num = reg.rsi;
			Process* proc = search_process((uint32_t)pid);
			if(!proc){
				reg.rax = -EINVAL;
				goto sys_call_finally;
			}
			proc->sig_pending = sig_num;
			reg.rax = 0;
		}else if(reg.rax == 101){
			// int sys_set_signal_handler(void (*handler)(uint64_t));
			uint64_t handler = reg.rdi;
			current_process->sig_handler = handler;
			ret = 0; // don't even need to context switch
		}else if(reg.rax == 102){
			// int64_t sys_open(void *pathname, uint64_t flags);
			char* path = (char*)(reg.rdi);
			int flag = (int)(reg.rsi);
			// translate relative path
			char* abs_path = calculate_path(current_process->workdir, path);
			if(flag & 0b100){ // create the file
				inode* node = search_file_in_disk(abs_path);
				if(!node){
					if(!create_file_in_disk(abs_path)){
						// failed to create
						kprintf("DEBUG: failed to create file\n");
						reg.rax = (uint64_t)-EIO;
						goto open_failed;
					}
				}else{
					sf_free(node);
				}
			}
			int i=0;
			for(;i<FD_SIZE; i++){
				if(!current_process->open_fd[i]) break;
			}
			if(i == FD_SIZE){
				// no available fd, limit reached
				kprintf("DEBUG: fd limit reached\n");
				reg.rax = (uint64_t)-EMFILE;
				goto open_failed;
			}
			if((flag & 0b11) == 0){
				// read mode
				file_table_entry* opened = file_open_read(abs_path);
				if(!opened){
					kprintf("DEBUG: file_open_read failed\n");
					reg.rax = (uint64_t)-EIO;
					goto open_failed;
				}
				open_file_descriptor* new_fd = sf_malloc(sizeof(open_file_descriptor));
				new_fd->file_entry = opened;
				current_process->open_fd[i] = new_fd;
			}else if((flag & 0b11) == 1){
				// write mode
				file_table_entry* opened = file_open_write(abs_path);
				if(!opened){
					kprintf("DEBUG: file_open_write failed\n");
					reg.rax = (uint64_t)-EIO;
					goto open_failed;
				}
				open_file_descriptor* new_fd = sf_malloc(sizeof(open_file_descriptor));
				new_fd->file_entry = opened;
				current_process->open_fd[i] = new_fd;
			}else{
					kprintf("DEBUG: open flag error\n");
				reg.rax = (uint64_t)-EINVAL;
				goto open_failed;
			}
			reg.rax = i;
			goto sys_call_finally;
			open_failed: 
			sf_free(abs_path);
			goto sys_call_finally;
		}else if(reg.rax == 103){
			// int64_t sys_read(uint64_t fd, void *buf, uint64_t count);
			uint64_t fd = reg.rdi;
			uint64_t buffer = reg.rsi;
			uint64_t to_write = reg.rdx;
			if(fd >= FD_SIZE){
				reg.rax = (uint64_t)-EMFILE;
				goto sys_call_finally;
			}
			open_file_descriptor* file = current_process->open_fd[fd];
			if(!file || !file->file_entry){
				reg.rax = (uint64_t)-EBADF;
				goto sys_call_finally;
			}
			file_table_entry* file_entry = file->file_entry;
			if(file_entry->io_type != 2 && file_entry->io_type != 3 && file_entry->io_type != 5){
				reg.rax = (uint64_t)-EBADF;
				goto sys_call_finally;
			}
			int written = 0;
			while(1){
				written = file_read(file_entry, current_process, (uint8_t*)(uint64_t*)buffer, to_write);
				// assume the file pair has not been closed
				if(written == 0){
					// TODO: increment open count on file before sleep
					file_table_waiting* waiter = sf_calloc(sizeof(file_table_waiting), 1);
					waiter->waiter = current_process;
					if(!file_entry->first_waiters){
						file_entry->first_waiters = waiter;
					}else{
						file_table_waiting* cursor = file_entry->first_waiters;
						while(cursor->next){
							cursor = cursor->next;
						}
						cursor->next = waiter;
					}
					// set on_hold
					current_process->on_hold = 1;
					// sleep this process in kernel
					trigger_ctx_switch();
					kprintf("sssss");
					if(current_process->sig_pending){
						break;
					}
				}else{
					break;
				}
			}
			reg.rax = (uint64_t)written;
		}else if(reg.rax == 104){
			// int64_t sys_write(uint64_t fd, void *buf, uint64_t count);
			uint64_t fd = reg.rdi;
			uint64_t buffer = reg.rsi;
			uint64_t to_read = reg.rdx;
			if(fd >= FD_SIZE){
				reg.rax = (uint64_t)-EMFILE;
				goto sys_call_finally;
			}
			open_file_descriptor* file = current_process->open_fd[fd];
			if(!file || !file->file_entry){
				reg.rax = (uint64_t)-EBADF;
				goto sys_call_finally;
			}
			file_table_entry* file_entry = file->file_entry;
			if(file_entry->io_type != 1 && file_entry->io_type != 4 && file_entry->io_type != 6){
				reg.rax = (uint64_t)-EBADF;
				goto sys_call_finally;
			}
			int readed = file_write(file_entry, current_process, (uint8_t*)(uint64_t*)buffer, to_read);
			reg.rax = (uint64_t)readed;
		}else if(reg.rax == 105){
			// void sys_exit(int status);
			int status = (int)reg.rdi;
			current_process->on_hold = 1;
			current_process->terminated = 1;
			current_process->ret_value = status;
			ret = 1;
		}else if(reg.rax == 106){
			// int64_t sys_brk(uint64_t new_bp);
			uint64_t new_bp = reg.rdi;
			if(new_bp == 0){
				reg.rax = current_process->heap_break;
				// kprintf("DEBUG: brk returns(%x)\n", current_process->heap_start);
				goto sys_call_finally;
			}
			if(new_bp < current_process->heap_start){
				reg.rax = (uint64_t)-ENOMEM;
				// kprintf("warning: new_bp too small(%x)\n", new_bp);
				goto sys_call_finally;
			}
			current_process->heap_break = new_bp;
			// kprintf("heap_break changed to: %x\n", current_process->heap_break);
			reg.rax = 0;
		}else if(reg.rax == 107){
			// int64_t sys_unlink(char * pathname);
			// TODO
			reg.rax = 0;
		}else if(reg.rax == 108){
			// int64_t sys_chdir(char * path);
			char* path = (char*)reg.rdi;
			uint32_t len = strlen((char*)path);
			char* old_path = current_process->workdir;
			sf_free(old_path);
			char* new_path = sf_malloc(len+1);
			for(int i=0; path[i]; i++) new_path[i] = path[i];
			new_path[len-1] = 0;
			current_process->workdir = new_path;
			reg.rax = 0;
		}else if(reg.rax == 109){
			// int64_t sys_getdir(char* buffer, uint64_t size);
			char* buffer = (char*)(uint64_t*)reg.rdi;
			uint64_t size = reg.rsi;
			char* path = current_process->workdir;
			int i=0;
			for(; path[i] && i<size-1; i++) buffer[i] = path[i];
			reg.rax = (uint64_t)buffer;
		}else if(reg.rax == 110){
			// int64_t sys_fork();
			// store the context on process structure
			current_process->saved_reg = reg;
			kernel_space_task_file.type = TASK_FORK_PROCESS;
			kernel_space_handler_wrapper();
			Process* new_p = (Process*)kernel_space_task_file.ret[0];
			reg.rax = new_p->id;
			// kprintf("DEBUG: fork complete with new child. id: %d, cr3: %x\n", new_p->id, new_p->cr3);
			ret = 1; // also to refresh page table
		}else if(reg.rax == 111){
			// int64_t sys_exec(char* path, char* argv[], char* envp[]);
			// kprintf("syscall: exec called\n");
			char* path = (char*)(uint64_t*)reg.rdi;
			char** argv = (char**)(uint64_t*)reg.rsi;
			char** envp = (char**)(uint64_t*)reg.rdx;
			// translate relative path
			char* abs_path = calculate_path(current_process->workdir, path);
			// read the elf file sections into memory
			file_table_entry* file = file_open_read(abs_path); // TODO: this needs to be a kernel task
			if(!file){
				reg.rax = (uint64_t)-EACCES;
				goto sys_call_finally;
			}
			program_section* sections = read_elf(file);
			if(!sections){
				reg.rax = (uint64_t)-ENOEXEC;
				goto sys_call_finally;
			}
			// start construct initial stack
			uint64_t argc = 0;
			uint64_t size_need = 4*8;
			uint64_t pointer_ends = 3*8;
			for(char** c = argv; *c!=0; c++) {
				size_need += 9;
				size_need += strlen(*c);
				pointer_ends+=8;
				argc++;
			}
			for(char** c = envp; *c!=0; c++) {
				size_need += 9;
				size_need += strlen(*c);
				pointer_ends+=8;
			}
			uint64_t* initial_stack = sf_calloc(size_need, 1);
			uint64_t* stack_cur = initial_stack;
			uint8_t* str_cur = (uint8_t*)initial_stack + pointer_ends;
			stack_cur[0] = argc;
			stack_cur++;
			for(char** c = argv; *c!=0; c++) {
				uint64_t len = strlen(*c)+1;
				memcpy(str_cur, *c, len);
				stack_cur[0] = str_cur - (uint8_t*)initial_stack; // the addresses are relative now, should be changed to absolute later
				stack_cur++;
				str_cur += len;
			}
			stack_cur[0] = 0;
			stack_cur++;
			for(char** c = envp; *c!=0; c++) {
				uint64_t len = strlen(*c)+1;
				memcpy(str_cur, *c, len);
				stack_cur[0] = str_cur - (uint8_t*)initial_stack; // the addresses are relative now, should be changed to absolute later
				stack_cur++;
				str_cur += len;
			}
			stack_cur[0] = 0;
			stack_cur++;
			assert(stack_cur - initial_stack == pointer_ends>>3, "ASSERT: exec initial stack size error\n");
			// stack prepared, ready to replace program
			// kprintf("syscall: exec process replacing\n");
			kernel_space_task_file.type = TASK_REPLACE_PROCESS;
			kernel_space_task_file.param[0] = (uint64_t) current_process;
			kernel_space_task_file.param[1] = (uint64_t) sections;
			kernel_space_task_file.param[2] = (uint64_t) initial_stack;
			kernel_space_task_file.param[3] = (uint64_t) size_need;
			kernel_space_handler_wrapper();
			// kprintf("syscall: exec process replaced\n");
			// restore program sections
			// alloc data pages
			program_section* section_cursor = sections;
			while(section_cursor){
				uint64_t prog_disk = section_cursor->file_offset;
				uint64_t prog_mem = section_cursor->memory_offset;
				uint64_t to_read = 0;
				uint64_t remain = section_cursor->size;
				uint64_t mem_remain = section_cursor->mem_size;
				for(; remain != 0; 
					prog_disk += to_read, prog_mem += to_read, remain -= to_read, mem_remain -= to_read){
					to_read = math_min(4096, 4096-(prog_mem % 4096), remain, (uint64_t)-1);
					// void* new_page = add_page_for_process(new_p, prog_mem, 1, 2);
					file_set_offset(file, prog_disk);
					if(file_read(file, current_process, (uint8_t*)(uint64_t*)prog_mem, to_read) < to_read){
						kprintf("ERROR: cannot read enough segment from elf file\n");
						while(1); // TODO: clean up and return instead of hung
					}
				}
				if(mem_remain != 0) {
					for(; mem_remain != 0; prog_mem++, mem_remain--){
						if(prog_mem % 0x1000 == 0){
							assert(0, "assert: exec zero page alloc failed\n");
						}
						*(uint8_t*)(uint64_t*)(prog_mem) = 0;
					}
				}
				section_cursor = section_cursor->next;
			}
			// copy initial stack
			uint64_t* rsp = (uint64_t*)(reg.ret_rsp);
			memcpy(rsp, initial_stack, size_need);
			uint64_t* rsp_c = rsp+1;
			for(; *rsp_c != 0; rsp_c++){
				*rsp_c = *rsp_c + (uint64_t)rsp;
			}
			rsp_c++;
			for(; *rsp_c != 0; rsp_c++){
				*rsp_c = *rsp_c + (uint64_t)rsp;
			}
			// done
			sf_free(abs_path);
			// kprintf("syscall: exec process loaded\n");
			ret = 1;
		}else if(reg.rax == 112){
			// int64_t sys_wait(int64_t pid, int* status);
			uint32_t pid = (uint32_t)reg.rdi;
			int* status_ret = (int*)reg.rsi;
			// TODO: check if pid exist
			current_process->id_wait_for = pid;
			current_process->on_hold = 1;
			current_process->id_wait_for_p = 0;
			trigger_ctx_switch();
			if(current_process->id_wait_for_p){
				status_ret[0] = current_process->id_wait_for_p->ret_value;
			}
			sf_free(current_process->id_wait_for_p);
			reg.rax = 0;
		}else if(reg.rax == 113){
			// void sys_pause(uint64_t nano_second);
			kernel_space_task_file.type = TASK_REG_WAIT;
			kernel_space_task_file.param[0] = reg.rdi * PIT_FREQUENCY / 1000000;
			kernel_space_handler_wrapper();
			current_process->on_hold = 1;
			// kprintf("DEBUG: kernel wait starts\n");
			__asm__ volatile ("int $0x81;");
			// kprintf("DEBUG: kernel wait ends\n");
			reg.rax = 0;
		}else if(reg.rax == 114){
			// int64_t sys_getpid();
			reg.rax = current_process->id;
		}else if(reg.rax == 115){
			// int64_t sys_getppid();
			if(current_process->parent) reg.rax = current_process->parent->id;
			else reg.rax = 0;
		}else if(reg.rax == 116){
			// int64_t sys_lseek(uint64_t fd, uint64_t offset);
			uint64_t fd = reg.rdi;
			uint64_t offset = reg.rsi;
			if(fd >= FD_SIZE){
				reg.rax = (uint64_t)-EMFILE;
				goto sys_call_finally;
			}
			open_file_descriptor* file = current_process->open_fd[fd];
			if(!file || !file->file_entry){
				reg.rax = (uint64_t)-EBADF;
				goto sys_call_finally;
			}
			file_table_entry* file_entry = file->file_entry;
			file_entry->offset = offset;
			reg.rax = 0;
		}else if(reg.rax == 117){
			// int64_t sys_mkdir(char* path);
			// TODO
			reg.rax = 0;
		}else if(reg.rax == 118){
			// int64_t sys_pipe(int fd[2]);
			int* fd = (int*)reg.rdi;
			// check if fd is writable
			file_table_entry* tmp[2];
			open_file_descriptor** open_fd = current_process->open_fd;
			int outfd = 0;
			for(;outfd<FD_SIZE && open_fd[outfd]; outfd++);
			if(outfd == FD_SIZE) {
				reg.rax = (uint64_t)-EMFILE;
				goto sys_call_finally;
			}
			int infd = 0;
			for(;infd<FD_SIZE && open_fd[infd]; infd++);
			if(infd == FD_SIZE) {
				reg.rax = (uint64_t)-EMFILE;
				goto sys_call_finally;
			}
			generate_entry_pair(tmp);
			open_fd[outfd] = sf_malloc(sizeof(open_file_descriptor));
			open_fd[infd] = sf_malloc(sizeof(open_file_descriptor));
			open_fd[outfd]->file_entry = tmp[0];
			open_fd[infd]->file_entry = tmp[1];
			fd[0] = outfd;
			fd[1] = infd;
			reg.rax = 0;
		}else if(reg.rax == 119){
			// int64_t sys_list_files(char* dir_path, struct dirent* write_to, int index);
			char* dir_path = (char*)reg.rdi;
			struct dirent* write_to = (struct dirent*)reg.rsi;
			int index = (int)reg.rdx;
			char* abs_path = calculate_path(current_process->workdir, dir_path);
			dirent_sys de = list_next_file(abs_path, index);
			if(!de.result){
				reg.rax = (uint64_t)-1;
				sf_free(abs_path);
				goto sys_call_finally;
			}
			int name_len = strlen(de.name);
			memcpy(write_to->d_name, de.name, name_len+1);
			write_to->d_ino = de.inode;
			write_to->d_off = 0;
			write_to->d_reclen = name_len;
			sf_free(de.name);
			sf_free(abs_path);
			reg.rax = 0;
		}else if(reg.rax == 120){
			// int64_t sys_close(uint64_t fd);
			uint64_t fd = reg.rdi;
			if(fd >= FD_SIZE){
				reg.rax = (uint64_t)-EMFILE;
				goto sys_call_finally;
			}
			file_close(current_process->open_fd[fd]->file_entry);
			sf_free(current_process->open_fd[fd]);
			current_process->open_fd[fd] = 0;
			reg.rax = 0;
		}else if(reg.rax == 121){
			// int64_t sys_sig_return();
			if(!current_process->sig_saved_reg.int_num){
				// process is not in signal handler, abort syscall
				goto sys_call_finally;
			}
			reg = current_process->sig_saved_reg;
			current_process->sig_saved_reg.int_num = 0;
			ret = 0; //  maybe the context switch is not needed
		}else if(reg.rax == 122){
			// int sys_dup(int oldfd, int newfd);
			uint64_t oldfd = reg.rdi;
			uint64_t newfd = reg.rsi;
			if(oldfd > FD_SIZE || newfd > FD_SIZE){
				reg.rax = (uint64_t)-1;
				goto sys_call_finally;
			}
			if(current_process->open_fd[newfd] != 0){
				// TODO: close newfd instead
				reg.rax = (uint64_t)-1;
				goto sys_call_finally;
			}
			if(current_process->open_fd[oldfd] == 0){
				reg.rax = (uint64_t)-1;
				goto sys_call_finally;
			}
			open_file_descriptor* oldfde = current_process->open_fd[oldfd];
			open_file_descriptor* newfde = sf_calloc(sizeof(open_file_descriptor), 1);
			memcpy(newfde, oldfde, sizeof(open_file_descriptor));
			oldfde->file_entry->open_count++;
			reg.rax = 0;
			
		}else if(reg.rax == 123){
			// int sys_alarm(int seconds);
			uint64_t seconds = reg.rdi;
			register_to_trigger_alarm(current_process, seconds * PIT_FREQUENCY);
			
		}else if(reg.rax == 248){
			ret = 1;
		}else if(reg.rax == 249){
			// test_kernel_read_block
			uint64_t* user_buffer = (uint64_t*)reg.rdx; // buffer
			kernel_space_task_file.type = TASK_RW_DISK_BLOCK;
			kernel_space_task_file.param[0] = reg.rdi; // disk index
			kernel_space_task_file.param[1] = (uint64_t)current_process;
			kernel_space_task_file.param[2] = (uint64_t)reg.rsi; // LBA
			kernel_space_task_file.param[3] = (uint64_t)0;
			kernel_space_task_file.param[4] = 0;
			kernel_space_handler_wrapper();
			if(!kernel_space_task_file.ret[0]){
				reg.rax = (uint64_t)-1;
				goto sys_call_finally;
			}
			page_entry* read_to_page = (page_entry*)(uint64_t*)kernel_space_task_file.ret[1];
			current_process->on_hold = 1;
			// kprintf("DEBUG: kernel wait starts\n");
			__asm__ volatile ("int $0x81;");
			kernel_space_task_file.type = TASK_CP_PAGE_MALLOC;
			uint64_t* buffer = sf_malloc(4096);
			kernel_space_task_file.param[0] = (uint64_t)read_to_page;
			kernel_space_task_file.param[1] = (uint64_t)buffer;
			kernel_space_task_file.param[2] = 1; // page_to_malloc
			kernel_space_handler_wrapper();
			for(int i = 0; i<512; i++){
				user_buffer[i] = buffer[i];
			}
			sf_free(buffer);
			read_to_page->used_by = 0;
			reg.rax = (uint64_t)0;
			// kprintf("DEBUG: kernel wait ends\n");
			
		}else if(reg.rax == 250){
			// test_kernel_write_block
			uint64_t* user_buffer = (uint64_t*)reg.rdx; // buffer
			uint64_t* buffer = sf_malloc(4096);
			for(int i = 0; i<512; i++){
				buffer[i] = user_buffer[i];
			}
			kernel_space_task_file.type = TASK_RW_DISK_BLOCK;
			kernel_space_task_file.param[0] = reg.rdi; // disk index
			kernel_space_task_file.param[1] = (uint64_t)current_process;
			kernel_space_task_file.param[2] = (uint64_t)reg.rsi; // LBA
			kernel_space_task_file.param[3] = (uint64_t)buffer;
			kernel_space_task_file.param[4] = 1;
			kernel_space_handler_wrapper();
			sf_free(buffer);
			if(!kernel_space_task_file.ret[0]){
				reg.rax = (uint64_t)-1;
				goto sys_call_finally;
			}
			current_process->on_hold = 1;
			// kprintf("DEBUG: kernel wait starts\n");
			__asm__ volatile ("int $0x81;");
			reg.rax = (uint64_t)0;
			// kprintf("DEBUG: kernel wait ends\n");
			
		}else if(reg.rax == 251){
			// test_kernel_wait
			kernel_space_task_file.type = TASK_REG_WAIT;
			kernel_space_task_file.param[0] = reg.rdi * PIT_FREQUENCY;
			kernel_space_handler_wrapper();
			current_process->on_hold = 1;
			// kprintf("DEBUG: kernel wait starts\n");
			__asm__ volatile ("int $0x81;");
			// kprintf("DEBUG: kernel wait ends\n");
		}else if(reg.rax == 252){
			// test_print
			uint64_t num = reg.rdi;
			kprintf("%x", num);
		}else if(reg.rax == 253){
			// test_print
			char* string_addr = (char*)reg.rdi;
			if(!string_addr){
				kprintf("DEBUG: invalid string address, %x\n", string_addr);
				goto sys_call_finally;
			}
			for(int i=0; i<80; i++){
				if(*(string_addr+i) == 0) break;
				kprintf("%c", *(string_addr+i));
			}
		}else if(reg.rax == 254){
			// test_wait
			kernel_space_task_file.type = TASK_REG_WAIT;
			kernel_space_task_file.param[0] = reg.rdi * PIT_FREQUENCY;
			kernel_space_handler_wrapper();
			current_process->on_hold = 1;
			ret = 1;
		}else{
			kprintf("WARNING: syscall undefined called\n");
		}
		sys_call_finally:
		handle_sig_pending(&reg);
		// ret = 1; // will trigger context switch
		goto finally;
	}else if(reg.int_num == 13){
		kprintf("General Protection fault occurred. Fault num: %d\n", reg.err_num);
		kprintf("fault rip: %x\n", reg.ret_rip);
		current_process->on_hold = 1;
		current_process->terminated = 1;
		// while(1); // halt the system to figure out what's wrong
		ret = 1;
	}else if(reg.int_num == 14){
		// assert(0, "page fault wip\n");
		uint64_t program_cr3;
		__asm__ volatile("movq %%cr3, %0":"=r"(program_cr3):);
		uint64_t requested_addr;
		__asm__ volatile("movq %%cr2, %0":"=r"(requested_addr):);
		kprintf("DEBUG: PF. fn:%d, c3:%x, %d, c2:%x, rip:%x\n", reg.err_num, program_cr3, current_process->id, requested_addr, reg.ret_rip);
		assert(current_process != 0, "no process exist at page fault handler\n");
		// TODO: check if this is a kernel page fault
		kernel_space_task_file.type = TASK_PROC_PAGE_FAULT_HANDLE;
		kernel_space_task_file.param[0] = reg.err_num;
		kernel_space_task_file.param[1] = requested_addr;
		kernel_space_handler_wrapper();
		handle_sig_pending(&reg);
		ret = 1;
	}else{
		kprintf("Unhandled interrupt on vector: %d\n", reg.int_num);
		while(1); // halt the system to figure out what's wrong
	}
	goto finally;
	finally:
	return ret;
}

void kernel_space_handler(struct kernel_task_return_reg reg){
	if(kernel_space_task_file.type == TASK_TIMER_TICK){
		pic_tick_count++;
		test_tick_handle();
		check_disk_task();
		
	}else if(kernel_space_task_file.type == TASK_FORK_PROCESS){
		Process* new_p = fork_process(current_process);
		kernel_space_task_file.ret[0] = (uint64_t)new_p;
		
	}else if(kernel_space_task_file.type == TASK_REG_WAIT){
		uint64_t ticks = kernel_space_task_file.param[0];
		register_to_be_waken(current_process, ticks);
		
	}else if(kernel_space_task_file.type == TASK_PROC_PAGE_FAULT_HANDLE){
		uint64_t err_num = kernel_space_task_file.param[0];
		uint64_t requested_addr = kernel_space_task_file.param[1];
		if((err_num == 7 || err_num == 5) && check_and_handle_rw_page_fault(current_process, requested_addr)){
			// kprintf("DEBUG: shared page duplicated\n");
		}else if(requested_addr < current_process->heap_break && requested_addr >= current_process->heap_start){
			// if the requested address falls in a resonable range of heap
			add_page_for_process(current_process, requested_addr, 1, 2);
		}else if(requested_addr <= (uint64_t)process_initial_rsp && requested_addr >= (current_process->rsp_current-0x4000)){
			// if the requested address falls in a resonable range for stack growth
			add_page_for_process(current_process, requested_addr, 1, 2);
			current_process->rsp_current = requested_addr;
		}else{
			kprintf("unable to handle Page Fault for process: %d\n", current_process->id);
			if(current_process->sig_handler){
				current_process->sig_pending = SIGSEGV;
				current_process->on_hold = 0;
			}else{
				current_process->on_hold = 1;
				current_process->terminated = 1;
			}
			// while(1); // halt the system to figure out what's wrong
		}
		
	}else if(kernel_space_task_file.type == TASK_PROC_CLEANUP){
		Process* proc = (Process*)kernel_space_task_file.param[0];
		process_cleanup(proc);
		
	}else if(kernel_space_task_file.type == TASK_KEYBOARD_HANDLE){
		uint8_t c = asm_inb(0x60);
		handle_keyboard_scan_code(c);
		
	}else if(kernel_space_task_file.type == TASK_KBRK){
		uint64_t size = kernel_space_task_file.param[0];
		if(size == 0) {
			kernel_space_task_file.ret[0] = (uint64_t)_kbrk(0);
			return;
		}
		void* ret = _kbrk(4096);
		for(size-=4096;size>0; size -= 4096){
			_kbrk(4096);
		}
		kernel_space_task_file.ret[0] = (uint64_t)ret;
		
	}else if(kernel_space_task_file.type == TASK_REPLACE_PROCESS){
		Process* proc = (Process*)kernel_space_task_file.param[0];
		program_section* sections = (program_section*)kernel_space_task_file.param[1];
		uint64_t* initial_stack = (uint64_t*)kernel_space_task_file.param[2];
		uint64_t initial_stack_size = (uint64_t)kernel_space_task_file.param[3];
		replace_process(proc, sections, initial_stack, initial_stack_size);
		
	}else if(kernel_space_task_file.type == TASK_RW_DISK_BLOCK){
		uint64_t disk_i = kernel_space_task_file.param[0];
		Process* proc = (Process*)kernel_space_task_file.param[1];
		uint64_t LBA = kernel_space_task_file.param[2];
		uint64_t* buffer = (uint64_t*)kernel_space_task_file.param[3];
		uint64_t is_write = kernel_space_task_file.param[4];
		int result = 1;
		page_entry* page = 0;
		if(is_write){
			page_entry* r_from = find_free_page_entry();
			r_from->used_by = 2;
			for(int i = 0; i<512; i++){
				((uint64_t*)(r_from->base))[i] = buffer[i];
			}
			result = write_disk_block_req(disk_i, proc, LBA, r_from);
			r_from->used_by = 0;
			if(!result) kernel_space_task_file.ret[0] = 0;
		}else{
			page = read_disk_block_req(disk_i, proc, LBA);
			kernel_space_task_file.ret[1] = (uint64_t)page;
			if(!page) kernel_space_task_file.ret[0] = 0;
		}
		kernel_space_task_file.ret[0] = 1;
		
	}else if(kernel_space_task_file.type == TASK_CP_PAGE_MALLOC){
		page_entry* page = (page_entry*)(uint64_t*)kernel_space_task_file.param[0];
		uint64_t* page_addr = (uint64_t*)page->base;
		uint64_t* malloc_addr = (uint64_t*)kernel_space_task_file.param[1];
		uint64_t page_to_malloc = kernel_space_task_file.param[2];
		if(page_to_malloc){
			for(int i=0; i<512; i++){
				malloc_addr[i] = page_addr[i];
			}
		}else{
			for(int i=0; i<512; i++){
				page_addr[i] = malloc_addr[i];
			}
		}
	}
}




