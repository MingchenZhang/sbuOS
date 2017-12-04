#include <syscall_test.h>

char* _argv[] = {"test.c", "arg1", "-l", 0};
char* _envp[] = {"PATH=/bin", "HOME=/usr/root/home", 0};

int main(int argc, char**argv){
	sys_print_num((unsigned long)_argv);
	// while(1);
	if(sys_test_fork()){ // parent
		while(1);
	}else{ // child
		sys_print_num((unsigned long)(_argv[0]));
		sys_test_exec("test", _argv, _envp);
		sys_print("sys_test_exec returned!!!\n");
	}
}
