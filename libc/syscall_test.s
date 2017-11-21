.global sys_print
sys_print:
	movq $253, %rax
	int $0x80
	ret

.global sys_print_num
sys_print_num:
	movq $252, %rax
	int $0x80
	ret

.global sys_test_kernel_wait
sys_test_kernel_wait:
	movq $251, %rax
	int $0x80
	ret

.global sys_test_fork
sys_test_fork:
	movq $1, %rax
	int $0x80
	ret

.global sys_test_exit
sys_test_exit:
	movq $2, %rax
	int $0x80
	ret

.global sys_test_wait
sys_test_wait:
	movq $254, %rax
	int $0x80
	ret

.global sys_test_read
sys_test_read:
	movq $4, %rax
	int $0x80
	ret

.global sys_test_write
sys_test_write:
	movq $5, %rax
	int $0x80
	ret

