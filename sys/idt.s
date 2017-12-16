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
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbp
	
	# when changed: also change those: two lines below, stack creation offset in thread creation, idt.h reg struct
	movq %rsp,%rdi # set the first argument
	call save_current_state # pre save current process state in case isr_handler needs them
	movq %rsp,%rdi # set the first argument
	call isr_handler
	cmpq $1, %rax # if need to context switch
	jne AFTER_CONTEXT_SWITCH
	int $0x81
	.global AFTER_CONTEXT_SWITCH
	AFTER_CONTEXT_SWITCH:
	
	popq %rbp
	popq %r15
	popq %r14
	popq %r13
	popq %r12
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

# setup context switch handler
.global isr129
.type isr129, @function
isr129:
	pushq $0
	pushq $0x81
	pushq %rdi
	pushq %rsi
	pushq %rbx
	pushq %rdx
	pushq %rcx
	pushq %rax
	pushq %r8
	pushq %r9
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbp 
	
	movq %cr3, %rdi
	call save_current_cr3
	movq %rsp,%rdi
	call save_current_rsp
	call process_schedule
	call load_current_cr3
	movq %rax,%rbp
	call load_current_rsp
	movq %rax, %rsp
	movq %rbp, %cr3
	
	popq %rbp
	popq %r15
	popq %r14
	popq %r13
	popq %r12
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

.global kernel_space_handler_wrapper
.type kernel_space_handler_wrapper, @function
kernel_space_handler_wrapper:
	# get current rsp and put in register
	movq %rsp, %r8
	# get and set kernel handler stack start
	call get_rsp0_stack
	movq %rax, %rsp
	# push current rsp to stack
	pushq %r8
	# push current cr3 to stack
	movq %cr3, %r8
	pushq %r8
	# get and set kernel cr3 
	call get_kernel_cr3
	movq %rax, %cr3
	movq %rsp, %rdi # give the return point access
	# call the kernel_space_handler
	call kernel_space_handler
	# pop cr3 to register
	popq %r8
	movq %r8, %cr3
	# pop rsp to register
	popq %rsp
	retq
	