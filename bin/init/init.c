#include <syscall_test.h>

int main(int argc, char**argv){
	// while(1);
	if(sys_test_fork()){ // parent
		while(1);
	}else{ // child
		sys_test_exec("bin/fork_test");
		sys_print("sys_test_exec returned!!!\n");
	}
}
