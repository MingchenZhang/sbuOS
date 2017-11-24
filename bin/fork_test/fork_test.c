#include <syscall_test.h>

int main(int argc, char**argv){
	int volatile value = 1011;
	if(sys_test_fork()){
		if(sys_test_fork()){
			// parent
			for(int i=0; i<4; i++){
				sys_test_wait(3);
				sys_print("parent report: ");
				sys_print_num(value);
				sys_print("\n");
			}
			sys_test_exit();
		}else{
			// child 2
			sys_test_exec("bin/test");
			sys_print("sys_test_exec returned!!!\n");
			value+= 2;
			for(int i=0; i<2; i++){
				sys_test_wait(3);
				sys_print("child2 report: ");
				sys_print_num(value);
				sys_print("\n");
			}
			sys_test_exit();
		}
	}else{
		//child 1
		value++;
		for(int i=0; i<2; i++){
			sys_test_wait(3);
			sys_print("child1 report: ");
			sys_print_num(value);
			sys_print("\n");
		}
		sys_test_exit();
	}
	sys_test_exit();
}
