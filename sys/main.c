#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>
#include <sys/idt.h>
#include <sys/misc.h>
#include <sys/config.h>
#include <sys/pci.h>
#include <sys/elf64.h>
#include <sys/memory/phy_page.h>
#include <sys/memory/kmalloc.h>
#include <sys/thread/kthread.h>
#include <sys/terminal.h>

#define INITIAL_STACK_SIZE 4096
uint8_t initial_stack[INITIAL_STACK_SIZE]__attribute__((aligned(16)));
uint32_t* loader_stack;
extern char kernmem, physbase;

void init_timer(uint32_t frequency)
{
   uint32_t divider = 1193180 / frequency;
   asm_outb(0x43, 0x36); // start sending divider
   asm_outb(0x40, divider & 0xFF); // set low bit
   asm_outb(0x40, (divider>>8) & 0xFF); // set high bit
}

int check_bytes(void* ptr, uint8_t value, uint64_t size){
	uint8_t* ptr2 = ptr;
	for(uint64_t i=0; i<size; i++){
		if(ptr2[i] != value) return 0;
	}
	return 1;
}

void start(uint32_t *modulep, void *physbase, void *physfree)
{
	
	clear_screen();
	
	kprintf("physfree %p\n", (uint64_t)physfree);
	kprintf("physbase %p\n", (uint64_t)physbase);
	kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
	
	uint64_t cr0 = 0;
	__asm__ volatile("movq %%cr0, %0;":"=r"((uint64_t)cr0));
	kprintf("cr0: %p\n", cr0);

	init_idt();
	// init_pic(); // it seems pic has been configured

	init_timer(PIT_FREQUENCY);

	// kprintf("screen cleared\n");
	
	phy_page_init(modulep);
	kernel_page_table_init();
	init_terminal();
	init_tarfs(&_binary_tarfs_start, &_binary_tarfs_end);
	init_process();
	
	// char* buffer = (void*)0x1000000;
	// int64_t readed = tarfs_read("bin/cat", buffer, 512);
	// if(readed >=0 ){
		// kprintf("tarfs_read reads %d bytes\n", readed);
		// while(1) __asm__("hlt;");
	// }else{
		// kprintf("tarfs_read failed\n");
	// }
	
	// while(1) __asm__("hlt;");
	
	{
		program_section* result;
		if(!(result = read_elf_tarfs("bin/init"))) {
			kprintf("read_elf_tarfs failed\n");
			while(1) __asm__("hlt;");
		}
		spawn_process(result, "bin/init");
	}
	
	{
		program_section* result;
		if(!(result = read_elf_tarfs("bin/test"))) {
			kprintf("read_elf_tarfs failed\n");
			while(1) __asm__("hlt;");
		}
		spawn_process(result, "bin/test");
	}
	
	// {
		// program_section* result;
		// if(!(result = read_elf_tarfs("bin/test2"))) {
			// kprintf("read_elf_tarfs failed\n");
			// while(1) __asm__("hlt;");
		// }
		// spawn_process(result, "bin/test2");
	// }
	
	// {
		// program_section* result;
		// if(!(result = read_elf_tarfs("bin/test3"))) {
			// kprintf("read_elf_tarfs failed\n");
			// while(1) __asm__("hlt;");
		// }
		// spawn_process(result, "bin/test3");
	// }
	
	{
		program_section* result;
		if(!(result = read_elf_tarfs("bin/fork_test"))) {
			kprintf("read_elf_tarfs failed\n");
			while(1) __asm__("hlt;");
		}
		spawn_process(result, "bin/fork_test");
	}
	
	kprintf("threads created\n");
	
	// kprintf("init pci\n");
	// init_pci();
	// kprintf("init pci done\n");
	
	// add_kernel_thread_to_process_list(); // discarded because of ring support
	
	// enable the interrupt
	// __asm__ volatile("sti;");
	
	// set_tss_rsp(rsp0_stack+RSP0_STACK_SIZE);
	
	__asm__ volatile (
		"movq $255, %rdi;"
		"int $0x80;"
		); // switch to other process
	
	while(1) __asm__("hlt;");
}

void boot(void)
{
	// note: function changes rsp, local stack variables can't be practically used
	register char *temp1, *temp2;

	for(temp2 = (char*)0xb8001; temp2 < (char*)0xb8000+160*25; temp2 += 2) *temp2 = 7 /* white */;
		__asm__ volatile (
		"cli;"
		"movq %%rsp, %0;"
		"movq %1, %%rsp;"
		:"=g"(loader_stack)
		:"r"(&initial_stack[INITIAL_STACK_SIZE])
	);
	init_gdt();
	start(
		(uint32_t*)((char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase),
		(uint64_t*)&physbase,
		(uint64_t*)(uint64_t)loader_stack[4]
	);
	for(
		temp1 = "!!!!! start() returned !!!!!", temp2 = (char*)0xb8000;
		*temp1;
		temp1 += 1, temp2 += 2
	) *temp2 = *temp1;
	while(1) __asm__ volatile ("hlt");
}
