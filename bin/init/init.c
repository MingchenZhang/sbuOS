#include <syscall_test.h>

int main(int argc, char**argv){
	while(1);
	if(sys_test_fork()){ // child
		while(1);
	}else{ // parent
		while(1);
	}
}
