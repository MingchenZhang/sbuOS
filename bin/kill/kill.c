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
	if(argc==2){
		kill(str_to_int(argv[1]), SIGKILL);
	}else if(argc == 3){
		kill(str_to_int(argv[1]), str_to_int(argv[2]));
	}else{
		printf("format: kill pid (signal_number)\n");
	}
	exit(0);
}