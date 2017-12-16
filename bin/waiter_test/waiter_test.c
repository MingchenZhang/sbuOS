#include <stdlib.h>
#include <unistd.h>
#include <debuglib.h>
#include <sys/ioctl.h>

char* _argv[] = {0};
char* _envp[] = {0};

int strlen(char* a){
	int ret = 0;
	for(;*a; a++, ret++);
	return ret;
}

int main(int argc, char *argv[], char *envp[]){
	// while(1);
	write(1, "waiter_test.c speaking\n", 23);
	write(1, "I have following args\n", 22);
	for(long i=0; argv[i]; i++){
		int len = strlen(argv[i]);
		write(1, argv[i], len);
		write(1, "\n", 1);
	}
	write(1, "I have following envp\n", 22);
	for(long i=0; envp[i]; i++){
		int len = strlen(envp[i]);
		write(1, envp[i], len);
		write(1, "\n", 1);
	}
	long child_pid = fork();
	if(child_pid){ // parent
		// _print("init parent \n");
		ioctl(0, TIOCSPGRP, child_pid);
		int status;
		_print("waiting for: ");
		_print_num(child_pid);
		_print("\n");
		wait(&status);
		int this_pid = getpid();
		ioctl(0, TIOCSPGRP, this_pid);
		_print("wait returned, status: ");
		_print_num(status);
		_print("\n");
		exit(0);
	}else{ // child
		// _print("init child \n");
		execvpe("sbush", _argv, _envp);
		// if(sys_test_fork()){
			// sys_test_exec("test3", _argv, _envp);
		// }else{
			// sys_test_exec("test_daemon", _argv, _envp);
		// }
		_print("sys_test_exec returned!!!\n");
	}
}
