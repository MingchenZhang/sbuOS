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

.global sys_test_open
sys_test_open:
	movq $6, %rax
	int $0x80
	ret

.global sys_test_exec
sys_test_exec:
	movq $3, %rax
	int $0x80
	ret

.global sys_test_set_signal_handler
sys_test_set_signal_handler:
	movq $7, %rax
	int $0x80
	ret

.global sys_test_sig_return
sys_test_sig_return:
	movq $8, %rax
	int $0x80
	ret

.global sys_test_sig
sys_test_sig:
	movq $9, %rax
	int $0x80
	ret

.global sys_test_alarm
sys_test_alarm:
	movq $10, %rax
	int $0x80
	ret
	
.global sys_test_dup2
sys_test_dup2:
	movq $11, %rax
	int $0x80
	ret

.global sys_test_ioctl
sys_test_ioctl:
	movq $12, %rax
	int $0x80
	ret

.global sys_test_sbrk
sys_test_sbrk:
	movq $13, %rax
	int $0x80
	ret

.global sys_test_read_block
sys_test_read_block:
	movq $249, %rax
	int $0x80
	ret

.global sys_test_write_block
sys_test_write_block:
	movq $250, %rax
	int $0x80
	ret