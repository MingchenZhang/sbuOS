#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]){
	for(long i=1; argv[i]; i++){
		printf("%s ", argv[i]);
	}
	putchar('\n');
	exit(0);
}