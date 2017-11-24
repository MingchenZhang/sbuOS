#include <syscall_test.h>

void stack(int count){
	char volatile space[3000];
	space[0] = count;
	if(count < 0){
		sys_print("done\n");
	}else{
		unsigned long rsp;
		__asm__ volatile ("movq %%rsp, %0": "=r"(rsp));
		sys_print("rsp reaches ");
		sys_print_num(rsp);
		sys_print("\n");
		stack(space[0]-1);
	}
}

int main(int argc, char**argv){
	char buffer[4];
	int readed;
	sys_test_write(1, "test.c\n", 7);
	while(1){
		readed = sys_test_read(0, buffer, 1);
		if(readed == 0) break;
		sys_test_write(1, buffer, 1);
	}
	sys_test_exit();
}
