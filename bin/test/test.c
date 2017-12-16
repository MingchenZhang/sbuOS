#include <stdlib.h>
#include <unistd.h>
#include <sys/defs.h>

int strlen(char* a){
	int ret = 0;
	for(;*a; a++, ret++);
	return ret;
}

int main(int argc, char *argv[], char *envp[]){
	// char buffer[128];
	write(1, "test.c speaking\n", 16);
	write(1, "I have following args\n", 22);
	for(long i=0; argv[i]; i++){
		int len = strlen(argv[i]);
		write(1, argv[i], len);
		write(1, "\n", 1);
	}
	write(1, "I have following envp\n", 22);
	for(long i=0; envp[i]; i++){
		int len = strlen(envp[i]);
		write(1, envp[i], len);
		write(1, "\n", 1);
	}
	exit(0);
}
