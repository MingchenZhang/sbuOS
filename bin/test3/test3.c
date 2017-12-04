#include <syscall_test.h>

int main(int argc, char**argv){
	unsigned char buffer[4096];
	for(int i=0; i<4096; i++){
		buffer[i] = 0xde;
	}
	sys_print("starts writing block\n");
	if(sys_test_write_block(1, 0, buffer) == -1){
		sys_print("fail to write\n");
		sys_test_exit();
	}else{
		sys_print("success to write\n");
	}
	for(int i=0; i<4096; i++){
		buffer[i] = 0;
	}
	sys_print("starts reading block\n");
	if(sys_test_read_block(1, 0, buffer) == -1){
		sys_print("fail to read\n");
		sys_test_exit();
	}else{
		sys_print("success to read\n");
	}
	for(int i=0; i<4096; i++){
		if(buffer[i] != 0xde){
			sys_print("read mismatch\n");
			sys_test_exit();
		}
	}
	sys_print("content checked\n");
	sys_test_exit();
}
