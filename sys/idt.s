	.global lidt
	.type lidt, @function
lidt:
	lidt (%rdi)
	retq

.macro MAKE_ISR_HANDLER num
	.global isr\num
	.type isr\num, @function
	isr\num:
		#addq $16 ,%rsp
		#iretq
		pushq $0
		pushq $\num
		jmp ISR_HANDLER_WRAPER
.ENDM

.macro MAKE_ISR_HANDLER_ERR num
	.global isr\num
	.type isr\num, @function
	isr\num:
		#addq $16 ,%rsp
		#iretq
		pushq $\num
		jmp ISR_HANDLER_WRAPER
.ENDM

ISR_HANDLER_WRAPER:
	pushq %rdi
	pushq %rsi
	pushq %rbp
	pushq %rbx
	pushq %rdx
	pushq %rcx
	pushq %rax
	pushq %rsp
	pushq %r8
	pushq %r9
	movq %rsp,%rdi # set the first argument
	call isr_handler
	popq %r9
	popq %r8
	popq %rsp
	popq %rax
	popq %rcx
	popq %rdx
	popq %rbx
	popq %rbp
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
