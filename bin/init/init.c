#include <stdlib.h>
#include <unistd.h>
#include <debuglib.h>
#include <sys/ioctl.h>

char* _argv[] = {"sbush", 0};
char* _envp[] = {"PATH=/bin", 0};

int main(int argc, char *argv[], char *envp[]){
	// while(1);
	long child_pid = fork();
	if(child_pid){ // parent
		// _print("init parent \n");
		ioctl(0, TIOCSPGRP, child_pid);
		while(1);
	}else{ // child
		// _print("init child \n");
		execvpe("/bin/sbush", _argv, _envp);
		// if(sys_test_fork()){
			// sys_test_exec("test3", _argv, _envp);
		// }else{
			// sys_test_exec("test_daemon", _argv, _envp);
		// }
		_print("sys_test_exec returned!!!\n");
	}
}
