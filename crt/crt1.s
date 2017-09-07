		.global _start
_start:	movq	0x0(%rsp), %r11
		imulq   $0x8, %r11 
		addq	%rsp, %r11
		movq	%r11, %rdx
		movq	%rsp, %rsi
		addq	$0x8, %rsi
		movq	%rsp, %rdi
		call	main
		movq	%rax, %rdi
		movq    $60, %rax
		syscall