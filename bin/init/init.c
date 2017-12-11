#include <syscall_test.h>

char* _argv[] = {"test.c", "arg1", "-l", 0};
char* _envp[] = {"PATH=/bin", "HOME=/usr/root/home", 0};

int main(int argc, char**argv){
	sys_print_num((unsigned long)_argv);
	// while(1);
	long child_pid = sys_test_fork();
	if(child_pid){ // parent
		sys_test_ioctl(0, TIOCSPGRP, child_pid);
		while(1);
	}else{ // child
		sys_test_exec("test3", _argv, _envp);
		// if(sys_test_fork()){
			// sys_test_exec("test3", _argv, _envp);
		// }else{
			// sys_test_exec("test_daemon", _argv, _envp);
		// }
		sys_print("sys_test_exec returned!!!\n");
	}
}
