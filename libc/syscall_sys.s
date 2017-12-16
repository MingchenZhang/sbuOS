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

.global sys_ioctl
sys_ioctl:
	movq $12, %rax
	int $0x80
	ret

.global sys_signal
sys_signal:
	movq $100, %rax
	int $0x80
	ret

.global sys_set_signal_handler
sys_set_signal_handler:
	movq $101, %rax
	int $0x80
	ret

.global sys_open
sys_open:
	movq $102, %rax
	int $0x80
	ret

.global sys_read
sys_read:
	movq $103, %rax
	int $0x80
	ret

.global sys_write
sys_write:
	movq $104, %rax
	int $0x80
	ret

.global sys_exit
sys_exit:
	movq $105, %rax
	int $0x80
	ret

.global sys_brk
sys_brk:
	movq $106, %rax
	int $0x80
	ret

.global sys_unlink
sys_unlink:
	movq $107, %rax
	int $0x80
	ret

.global sys_chdir
sys_chdir:
	movq $108, %rax
	int $0x80
	ret

.global sys_getdir
sys_getdir:
	movq $109, %rax
	int $0x80
	ret

.global sys_fork
sys_fork:
	movq $110, %rax
	int $0x80
	ret

.global sys_exec
sys_exec:
	movq $111, %rax
	int $0x80
	ret

.global sys_wait
sys_wait:
	movq $112, %rax
	int $0x80
	ret

.global sys_pause
sys_pause:
	movq $113, %rax
	int $0x80
	ret

.global sys_getpid
sys_getpid:
	movq $114, %rax
	int $0x80
	ret

.global sys_getppid
sys_getppid:
	movq $115, %rax
	int $0x80
	ret

.global sys_lseek
sys_lseek:
	movq $116, %rax
	int $0x80
	ret

.global sys_mkdir
sys_mkdir:
	movq $117, %rax
	int $0x80
	ret

.global sys_pipe
sys_pipe:
	movq $118, %rax
	int $0x80
	ret

.global sys_list_files
sys_list_files:
	movq $119, %rax
	int $0x80
	ret

.global sys_close
sys_close:
	movq $120, %rax
	int $0x80
	ret

.global sys_sig_return
sys_sig_return:
	movq $121, %rax
	int $0x80
	ret

.global sys_dup
sys_dup:
	movq $122, %rax
	int $0x80
	ret
	
.global sys_alarm
sys_alarm:
	movq $123, %rax
	int $0x80
	ret
	
.global sys_yield
sys_yield:
	movq $248, %rax
	int $0x80
	ret