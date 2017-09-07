		.global sys_exit
sys_exit:	movq $60,%rax # syscall 60: exit
		syscall
		ret
		
		.global sys_write
sys_write:	movq $1, %rax # syscall 1: write
		syscall
		ret
		
		.global sys_read
sys_read:	movq $0, %rax # syscall 0: read
		syscall
		ret
		
		.global sys_close
sys_close:	movq $3, %rax # syscall 3: close
		syscall
		ret
		
		.global sys_pipe
sys_pipe:	movq $22, %rax # syscall 22: pipe
		syscall
		ret
		
		.global sys_fork
sys_fork:	movq $57,%rax # syscall 57: fork
		syscall
		ret
		
		.global sys_dup2
sys_dup2:	movq $33, %rax # syscall 33: dup2
		syscall
		ret
		
		.global sys_execve
sys_execve:	movq $59, %rax # syscall 59: execve
		syscall
		ret
		
		.global sys_waitpid
sys_waitpid:movq $61, %rax # syscall 61: wait4
		syscall
		ret
		
		.global sys_mmap
sys_mmap:	movq $9, %rax # syscall 9: mmap
		syscall
		ret
		
		.global sys_chdir
sys_chdir:	movq $80, %rax # syscall 80: chdir
		syscall
		ret
		
		.global sys_brk
sys_brk:	movq $12, %rax # syscall 12: brk
		syscall
		ret
		
		.global sys_open
sys_open:	movq $2, %rax # syscall 2: open
		syscall
		ret
		
		.global sys_old_readdir # not working
sys_old_readdir:	movq $0x59, %rax # syscall 0x59:readdir
		syscall
		ret
		
		.global sys_getdents
sys_getdents:	movq $78, %rax # syscall 0x8d:sys_getdents
		syscall
		ret