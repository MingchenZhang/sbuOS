#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <debuglib.h>

int main(int argc, char *argv[], char *envp[]){
	_print("start allocating\n");
	char* space1 = malloc(2000);
	for(int i=0; i<2000; i++) space1[i] = -22;
	char* space2 = malloc(2000);
	for(int i=0; i<2000; i++) space2[i] = -23;
	char* space3 = malloc(2000);
	for(int i=0; i<2000; i++) space3[i] = -24;
	char* space4 = malloc(2000);
	for(int i=0; i<2000; i++) space4[i] = -25;
	char* space5 = malloc(2000);
	for(int i=0; i<2000; i++) space5[i] = -26;
	
	int child = fork();
	if(child){
		_print("parent start verifying\n");
		int i=0;
		for(i=0; i<2000 && space1[i] == -22; i++);
		if(i!=2000) goto failed;
		for(i=0; i<2000 && space2[i] == -23; i++);
		if(i!=2000) goto failed;
		for(i=0; i<2000 && space3[i] == -24; i++);
		if(i!=2000) goto failed;
		for(i=0; i<2000 && space4[i] == -25; i++);
		if(i!=2000) goto failed;
		for(i=0; i<2000 && space5[i] == -26; i++);
		if(i!=2000) goto failed;
			
		_print("parent verification succeed\n");
		exit(0);
		failed:
		_print("parent verification failed\n");
		exit(1);
	}else{
		for(int i=0; i<2000; i++) space1[i] = 22;
		for(int i=0; i<2000; i++) space2[i] = 23;
		for(int i=0; i<2000; i++) space3[i] = 24;
		for(int i=0; i<2000; i++) space4[i] = 25;
		for(int i=0; i<2000; i++) space5[i] = 26;
		
		_print("child changed\n");
		exit(0);
	}
	
}