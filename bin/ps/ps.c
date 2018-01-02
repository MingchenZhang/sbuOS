#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

int _strlen(char* a){
	int ret = 0;
	for(;*a; a++, ret++);
	return ret;
}

int str_to_int(char* str){
	int acc = 0;
	for(int i = _strlen(str)-1, j = 0; i>=0; i--, j++){
		acc = acc*10 + (str[j]-'0');
	}
	return acc;
}

int main(int argc, char *argv[], char *envp[]){
	int plist[32];
	int get;
	if((get = list_pid(plist, 32))<0){
		printf("fail to get process list\n");
		exit(1);
	}
	for(int i=0; i<get; i++){
		char buffer[128];
		int result = pid_name(buffer, plist[i], 128);
		if(result == 0){
			printf("pid: %d, name: %s\n", plist[i], buffer);
		}
	}
	exit(0);
}