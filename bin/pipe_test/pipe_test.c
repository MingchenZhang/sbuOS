#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <debuglib.h>

int main(int argc, char *argv[], char *envp[]){
	int pipe_fd[2];
	if(pipe(pipe_fd)<0){
		_print("fail to pipe\n");
		return 1;
	}
	char buffer[16];
	write(pipe_fd[1], "1234567890", 10);
	read(pipe_fd[0], buffer, 10);
	buffer[11] = 0;
	puts(buffer);
	_print("pipe done\n");
	return 0;
}