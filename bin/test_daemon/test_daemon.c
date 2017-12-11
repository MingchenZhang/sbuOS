#include <syscall_test.h>
#include <sys/defs.h>

int strlen(char* a){
	int ret = 0;
	for(;*a; a++, ret++);
	return ret;
}

int main(int argc, char**argv, char** envp){
	char buffer[128];
	sys_test_write(1, "test.c speaking\n", 16);
	sys_test_write(1, "I have following args\n", 22);
	for(long i=0; argv[i]; i++){
		int len = strlen(argv[i]);
		sys_test_write(1, argv[i], len);
		sys_test_write(1, "\n", 1);
	}
	sys_test_write(1, "I have following envp\n", 22);
	for(long i=0; envp[i]; i++){
		int len = strlen(envp[i]);
		sys_test_write(1, envp[i], len);
		sys_test_write(1, "\n", 1);
	}
	
	int readed;
	while(1){
		readed = sys_test_read(0, buffer, 1);
		if(readed == 0) break;
		sys_test_write(1, buffer, 1);
	}
	
	sys_test_exit();
}
