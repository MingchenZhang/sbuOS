#include <syscall_test.h>

void sig_handler(unsigned long sig_num){
	sys_print("signal triggered: ");
	sys_print_num(sig_num);
	sys_print("\n");
	if(sig_num == SIGSEGV){
		sys_print("SIGSEGV triggered, exiting\n");
		sys_test_exit();
	}else if(sig_num == SIGINT){
		sys_print("SIGINT triggered, exiting\n");
		sys_test_exit();
	}
	return sys_test_sig_return();
}

int main(int argc, char**argv){
	sys_test_set_signal_handler(sig_handler);
	sys_test_alarm(4);
	for(long i=0; ; i++){
		for(long j=0; j<100000000; j++);
		sys_print("test3: beep\n");
		// if(i == 4){
			// *((unsigned long*)(0x0)) = 1000;
		// }
	}
	sys_print("exiting\n");
	sys_test_exit();
}
