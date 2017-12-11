#include <syscall_test.h>

int main(int argc, char**argv){
	unsigned long sig_num = 3;
	sys_test_kernel_wait(1);
	sys_print("signal sent\n");
	sys_test_sig(1, sig_num);
	sys_test_exit();
}
