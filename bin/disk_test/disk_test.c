#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

int main(int argc, char *argv[], char *envp[]){
	if(argc<3){
		printf("3 arguments required\n");
		exit(1);
	}
	char path[128] = "/d/";
	int i=0;
	for(; argv[2][i]; i++){
		path[i+3] = argv[2][i];
	}
	path[i+3] = 0;
	printf("disk_test.c: disk file to create: %s\n", path);
	if(argv[1][0] == 'w'){
		int fd = open(path, O_CREAT | O_WRONLY);
		write(fd, "test_write ", 11);
	}else{
		int fd = open(path, O_CREAT | O_RDONLY);
		char buffer[8];
		for(;;){
			if(read(fd, buffer, 1) == 0){
				exit(0);
			}
			write(1, buffer, 1);
		}
	}
	
	return 0;
}