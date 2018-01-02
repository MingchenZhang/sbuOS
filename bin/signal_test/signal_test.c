#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <debuglib.h>

void sig_handler(int sig_num){
	printf("get signal %d\n", sig_num);
	// _print("got signal\n");
}

int main(int argc, char *argv[], char *envp[]){
	signal(SIGALRM, sig_handler);
	signal(SIGINT, sig_handler);
	alarm(3);
	while(1);
	exit(0);
}