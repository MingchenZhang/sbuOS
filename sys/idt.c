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
	// apply the changes
	lidt(&idt_ptr);
}

static inline void test_tick_handle(){
	tick_timer_update();
	if(pic_tick_count%PIT_FREQUENCY == 0){
		update_topright_display();
	}
}

void isr_handler(handler_reg volatile reg){
	if(reg.int_num == IRQ0){ // timer interrupt (from PIT)
		__asm__ volatile("movq %0, %%cr3"::"r"((uint64_t)kernel_page_table_PML4)); // return to kernel page table for complex op
		pic_tick_count++;
		test_tick_handle();
	}else if(reg.int_num == IRQ1){ // keyboard interrupt
		// kprintf("keyboard interrupt received\n");
		uint8_t c = asm_inb(0x60);
		// kprintf("Keyboard scan code: %x\n", c);
		handle_keyboard_scan_code(c);
	}else if(reg.int_num == 0x80){
		if(reg.rdi == 255){
			kprintf("switch to ring 3 and enable interrupt up on return\n");
			reg.cs = USER_CODE_SEGMENT_SELECTOR;
			reg.ss = USER_STACK_SEGMENT_SELECTOR;
			reg.eflags = 0x200216;
			goto finally;
		}
		__asm__ volatile("movq %0, %%cr3"::"r"((uint64_t)kernel_page_table_PML4)); // return to kernel page table for complex op
		// syscall return must be written back to reg.rax
		if(reg.rdi == 1){
			kprintf("syscall: fork called\n");
			Process* new_p = fork_process(current_process);
			reg.rax = new_p->id;
			kprintf("DEBUG: fork complete\n");
		}else if(reg.rdi == 100){
			kprintf("syscall 1\n");
		}else if(reg.rdi == 101){
			kprintf("syscall 2\n");
		}else if(reg.rdi == 102){
			kprintf("syscall 3\n");
		}else if(reg.rdi == 252){
			// test_print
			uint64_t num = reg.rsi;
			kprintf("%x", num);
		}else if(reg.rdi == 253){
			// test_print
			char* string_addr = (char*)reg.rsi;
			if(!string_addr){
				kprintf("DEBUG: invalid string address, %x\n", reg.rdx);
				goto sys_call_finally;
			}
			for(int i=0; i<80; i++){// translation can be better performed
				char* phy_addr = (char*)translate_cr3(current_process->cr3, (uint64_t)(string_addr+i));
				if(!phy_addr) break;
				if(*phy_addr == 0) break;
				kprintf("%c", *phy_addr);
			}
		}else if(reg.rdi == 254){
			// test_wait
			uint64_t ticks = reg.rsi * PIT_FREQUENCY;
			register_to_be_waken(current_process, ticks);
			current_process->on_hold = 1;
		}else{
			kprintf("WARNING: syscall undefined called\n");
		}
		sys_call_finally:
		__asm__ volatile("movq %0, %%cr3"::"r"((uint64_t)current_process->cr3)); 
	}else if(reg.int_num == 13){
		kprintf("General Protection fault occurred. Fault num: %d\n", reg.err_num);
		while(1); // halt the system to figure out what's wrong
	}else if(reg.int_num == 14){
		uint64_t program_cr3;
		__asm__ volatile("movq %%cr3, %0":"=r"(program_cr3):);
		uint64_t requested_addr;
		__asm__ volatile("movq %%cr2, %0":"=r"(requested_addr):);
		kprintf("DEBUG: Page Fault occurred. Fault num:%d, cr3:%x, cr2:%x\n", reg.err_num, program_cr3, requested_addr);
		// TODO: check if this is a kernel page fault
		__asm__ volatile("movq %0, %%cr3"::"r"((uint64_t)kernel_page_table_PML4)); // return to kernel page table for complex op
		if(reg.err_num == 7 && check_and_handle_rw_page_fault(current_process, requested_addr)){
			kprintf("DEBUG: shared page duplicated\n");
		}else if(requested_addr < current_process->heap_break && requested_addr > current_process->heap_start){
			// if the requested address falls in a resonable range of heap
			add_page_for_process(current_process, requested_addr, 1);
		}else if(requested_addr <= PROCESS_INITIAL_RSP && requested_addr >= (current_process->rsp_current-8192)){
			// if the requested address falls in a resonable range for stack growth
			add_page_for_process(current_process, requested_addr, 1);
			current_process->rsp_current = requested_addr;
		}else{
			kprintf("unable to handle Page Fault\n");
			// TODO: terminate the process
			while(1); // halt the system to figure out what's wrong
		}
		__asm__ volatile("movq %0, %%cr3"::"r"((uint64_t)current_process->cr3));
	}else{
		kprintf("Unhandled interrupt on vector: %d\n", reg.int_num);
		while(1); // halt the system to figure out what's wrong
	}
	finally:
	if(reg.int_num >= 32 && reg.int_num < 48){ // a PIC interrupt
		if (reg.int_num >= 40){ // it came from PIC
			asm_outb(0xA0, 0x20); // Send EOI to slave.
		}
		asm_outb(0x20, 0x20);// Send EOI to master
	}
}
