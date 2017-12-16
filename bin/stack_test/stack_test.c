#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <debuglib.h>

char* _argv[] = {0};
char* _envp[] = {0};

int main(int argc, char *argv[], char *envp[]){
	_print("start allocating\n");
	short buffer[6000];
	for(int i=0; i<6000; i++){
		buffer[i] = 8888-i;
	}
	
	int child = fork();
	if(child){
		_print("parent start execute\n");
		execvpe("cat", _argv, _envp);
	}else{
		_print("child start verifying\n");
		for(int i=0; i<6000; i++){
			buffer[i] = 6666-i;
			if(i%1000 == 0) yield();
		}
		for(int i=0; i<6000; i++){
			if(buffer[i] != 6666-i){
				goto c_failed;
			}
			if(i%1000 == 0) yield();
		}
		_print("child verification succeed\n");
		exit(0);
		c_failed:
		_print("child verification failed\n");
		exit(1);
	}
	
}