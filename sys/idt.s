	.global lidt
	.type lidt, @function
lidt:
	lidt (%rdi)
	retq

.macro MAKE_ISR_HANDLER num
	.global isr\num
	.type isr\num, @function
	isr\num:
		pushq $0
		pushq $\num
		jmp ISR_HANDLER_WRAPER
.ENDM

.macro MAKE_ISR_HANDLER_ERR num
	.global isr\num
	.type isr\num, @function
	isr\num:
		pushq $\num
		jmp ISR_HANDLER_WRAPER
.ENDM

ISR_HANDLER_WRAPER:
	pushq %rdi
	pushq %rsi
	pushq %rbx
	pushq %rdx
	pushq %rcx
	pushq %rax
	pushq %r8
	pushq %r9
	pushq %rbp 
	# when changed: also change those: two lines below, stack creation offset in thread creation, idt.h reg struct
	movq %rsp,%rdi # set the first argument
	call save_current_state # pre save current process state in case isr_handler needs them
	movq %rsp,%rdi # set the first argument
	call isr_handler
	cmpq $128, 72(%rsp) # if syscall
	je ISR_HANDLER_WRAPER_context_switch
	cmpq $32, 72(%rsp) # if timer
	je ISR_HANDLER_WRAPER_context_switch
	jmp ISR_HANDLER_WRAPER_not_context_switch
	ISR_HANDLER_WRAPER_context_switch:
	call process_schedule
	# start context switch
	movq 112(%rsp), %rdi
	call save_previous_rsp
	movq 88(%rsp), %rdi
	call save_previous_rip
	movq %rsp,%rdi # set the first argument
	call save_previous_registers
	
	call load_current_cr3
	movq %rax, %r9
	call load_current_rsp
	movq %rax, 112(%rsp)
	call load_current_rip
	movq %rax, 88(%rsp)
	movq %rsp,%rdi # set the first argument
	call load_previous_registers
	movq %r9, %cr3 # context switch
	ISR_HANDLER_WRAPER_not_context_switch:
	popq %rbp
	popq %r9
	popq %r8
	popq %rax
	popq %rcx
	popq %rdx
	popq %rbx
	popq %rsi
	popq %rdi
	addq $16 ,%rsp
	iretq

MAKE_ISR_HANDLER 0
MAKE_ISR_HANDLER 1
MAKE_ISR_HANDLER 2
MAKE_ISR_HANDLER 3
MAKE_ISR_HANDLER 4
MAKE_ISR_HANDLER 5
MAKE_ISR_HANDLER 6
MAKE_ISR_HANDLER 7
MAKE_ISR_HANDLER_ERR 8
MAKE_ISR_HANDLER 9
MAKE_ISR_HANDLER_ERR 10
MAKE_ISR_HANDLER_ERR 11
MAKE_ISR_HANDLER_ERR 12
MAKE_ISR_HANDLER_ERR 13
MAKE_ISR_HANDLER_ERR 14
MAKE_ISR_HANDLER 15
MAKE_ISR_HANDLER 16
MAKE_ISR_HANDLER_ERR 17
MAKE_ISR_HANDLER 18
MAKE_ISR_HANDLER 19
MAKE_ISR_HANDLER 20
MAKE_ISR_HANDLER 21
MAKE_ISR_HANDLER 22
MAKE_ISR_HANDLER 23
MAKE_ISR_HANDLER 24
MAKE_ISR_HANDLER 25
MAKE_ISR_HANDLER 26
MAKE_ISR_HANDLER 27
MAKE_ISR_HANDLER 28
MAKE_ISR_HANDLER 29
MAKE_ISR_HANDLER_ERR 30
MAKE_ISR_HANDLER 31
MAKE_ISR_HANDLER 32
MAKE_ISR_HANDLER 33
MAKE_ISR_HANDLER 128
