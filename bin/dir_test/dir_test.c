#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>

int main(int argc, char *argv[], char *envp[]){
	// char buffer[16];
	printf("dir_test stated\n");
	DIR* dir = opendir("./");
	if(!dir){
		printf("dir_test failed to open dir\n");
	}
	struct dirent *pDirent;
	while((pDirent = readdir(dir)) != NULL){
		printf("[%s]\n", pDirent->d_name);
	}
	printf("dir_test list complete\n");
	free(dir);
	return 0;
}