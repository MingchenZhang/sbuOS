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

#define INITIAL_STACK_SIZE 4096
#define RSP0_STACK_SIZE 0
uint8_t initial_stack[INITIAL_STACK_SIZE]__attribute__((aligned(16)));
uint8_t rsp0_stack[RSP0_STACK_SIZE]__attribute__((aligned(16))); // used as a clean stack to switch back to ring0
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

void test_func_1(){
	for(long long i=0; ;i++){
		if(i%0x20000000 == 0){
			// kprintf("test_func_1 reach %x\n", i);
			__asm__ volatile (
			"movq $100, %rdi;"
			"int $0x80;");
		}
	}
}

void test_func_2(){
	for(long long i=0; ;i++){
		if(i%0x17000000 == 0){
			// kprintf("test_func_2 reach %x\n", i);
			__asm__ volatile (
			"movq $101, %rdi;"
			"int $0x80;");
		}
	}
}

void test_func_3(){
	for(long long i=0; ;i++){
		if(i%0x15000000 == 0){
			// kprintf("test_func_3 reach %x\n", i);
			__asm__ volatile (
			"movq $102, %rdi;"
			"int $0x80;");
		}
	}
}

void test_func(){
	int volatile a = 43;
	a+=5;
	__asm__ volatile ("int $1;");
	kprintf("", a);
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
	init_tarfs(&_binary_tarfs_start, &_binary_tarfs_end);
	
	// for(int i=0; i<512; i++){
		// kbrk(4096);
	// }
	// void* point0 = kbrk(4096);
	// memset(point0, 0b10101010, 4096);
	// void* point1 = kbrk(4096);
	// memset(point1, 0b01010101, 4096);
	// void* point2 = kbrk(4096);
	// memset(point2, 0b10101010, 4096);
	// kprintf("1:%p, 2:%p, 3:%p\n", point0, point1, point2);
	// if(!check_bytes(point0, 0b10101010, 4096)) kprintf("Wrong !!\n");
	// if(!check_bytes(point1, 0b01010101, 4096)) kprintf("Wrong !!\n");
	// if(!check_bytes(point2, 0b10101010, 4096)) kprintf("Wrong !!\n");
	// kprintf("point2: %x\n", *(uint64_t*)point0);
	
	// int* num1 = sf_malloc(4000);
	// *num1 = 3432;
	// kprintf("num1 = %d, at %p\n", *num1, num1);
	// sf_free(num1);
	// int* num2 = sf_malloc(4);
	// *num2 = 67867;
	// kprintf("num1 = %d, at %p\n", *num2, num2);
	
	// test_spawn_process(&test_func_1, (uint64_t)test_func_2-(uint64_t)test_func_1);
	// test_spawn_process(&test_func_2, (uint64_t)test_func_3-(uint64_t)test_func_2);
	// test_spawn_process(&test_func_3, (uint64_t)test_func-(uint64_t)test_func_3);
	// kprintf("threads created\n");
	// __asm__ volatile ("int $0x80;");
	
	// char* buffer = (void*)0x1000000;
	// int64_t readed = tarfs_read("bin/cat", buffer, 512);
	// if(readed >=0 ){
		// kprintf("tarfs_read reads %d bytes\n", readed);
		// while(1) __asm__("hlt;");
	// }else{
		// kprintf("tarfs_read failed\n");
	// }
	
	program_section* result1;
	if(!(result1 = read_elf_tarfs("bin/test"))) {
		kprintf("read_elf_tarfs failed\n");
		while(1) __asm__("hlt;");
	}
	test_spawn_process_elf(result1, "bin/test");
	program_section* result2;
	if(!(result2 = read_elf_tarfs("bin/test2"))) {
		kprintf("read_elf_tarfs failed\n");
		while(1) __asm__("hlt;");
	}
	test_spawn_process_elf(result2, "bin/test2");
	program_section* result3;
	if(!(result3 = read_elf_tarfs("bin/test3"))) {
		kprintf("read_elf_tarfs failed\n");
		while(1) __asm__("hlt;");
	}
	test_spawn_process_elf(result3, "bin/test3");
	kprintf("threads created\n");
	
	// kprintf("init pci\n");
	// init_pci();
	// kprintf("init pci done\n");
	
	// add_kernel_thread_to_process_list(); // discarded because of ring support
	
	// enable the interrupt
	// __asm__ volatile("sti;");
	
	set_tss_rsp(rsp0_stack+RSP0_STACK_SIZE);
	
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
