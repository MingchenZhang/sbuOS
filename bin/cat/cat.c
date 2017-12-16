#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]){
	char buffer[16];
	printf("cat stated, %d, %s, %x, %p\n", 123, "test", 0x444, main);
	while(1){
		if(read(0, buffer, 1)<0){
			write(1, "cat: EOF reached\n", 17);
			exit(0);
		}
		write(1, buffer, 1);
	}
}