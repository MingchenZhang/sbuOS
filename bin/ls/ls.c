#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <debuglib.h>

int main(int argc, char *argv[], char *envp[]){
	DIR* dir;
	struct dirent *pDirent;
	if(argc < 2){//list current
		dir = opendir("./");
		if(dir == 0){
			puts("fail to open directory");
			return 1;
		}
	}else{//list specified
		dir = opendir(argv[1]);
		if(dir == 0){
			puts("fail to open directory");
			return 1;
		}
	}
	while((pDirent = readdir(dir)) != NULL){
		// _print(pDirent->d_name);
		// _print("\n");
		printf("%s\n", pDirent->d_name);
	}
	closedir(dir);
	return 0;
}