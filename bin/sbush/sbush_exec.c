// libc used: pipe, fork, dup2, execvp, waitpid, chdir, exit
#include <stdio.h> // puts
#include <stdlib.h> // exit
#include <unistd.h> // pipe, fork, dup2, execvp, chdir
#include <debuglib.h>

#include "include/sbush_process.h"
#include "include/sbush_exec.h"

int build_in_exec(Process* proc);

void execute(Job job) {
	Process* proc_cursor;
	
	// create process pipe pipe() pair
	proc_cursor = job.first_process;
	while(proc_cursor->next){
		int pipePair[2];
		if(pipe(pipePair) < 0){
			puts("failed on pipe()\n");
			return;
		}
		proc_cursor->fd_out = pipePair[1];
		proc_cursor->next->fd_in = pipePair[0];
		proc_cursor = proc_cursor->next;
	}
	
	proc_cursor = job.first_process;
	while(proc_cursor){
		// check build in
		if(build_in_exec(proc_cursor) >= 0){
			proc_cursor = proc_cursor->next;
			continue;
		}
		
		// fork
		int pid;
		if((pid = fork()) == 0){ // child route
			// dup2 pipe redirection
			if(proc_cursor->fd_in >= 0){
				dup2(proc_cursor->fd_in, 0); // stdin
				close(proc_cursor->fd_in);
			}
			if(proc_cursor->fd_out >= 0){
				dup2(proc_cursor->fd_out, 1); // stdout
				close(proc_cursor->fd_out);
			}
			
			// exec
			if(execvp(proc_cursor->path, proc_cursor->argv) <0){
				//error("fail to execute %s. errno: %d\n", procCursor->path, errno);
				puts("failed on execvp()\n");
				//perror("reason");
			}
			exit(1);
		}else{ // parent route
			if(proc_cursor->fd_in >= 0){
				close(proc_cursor->fd_in);
			}
			if(proc_cursor->fd_out >= 0){
				close(proc_cursor->fd_out);
			}
			proc_cursor->process_id = pid;
			proc_cursor = proc_cursor->next;
		}
	}
	
	// wait forked children
	if(!job.isBackground){
		proc_cursor = job.first_process;
		while(proc_cursor && proc_cursor->process_id >= 0){
			int status;
			if(waitpid(proc_cursor->process_id, &status) < 0){ // WUNTRACED == 2
				puts("failed on waitpid()\n");
				//perror("reason");
			}
			proc_cursor = proc_cursor->next;
		}
	}
	
}

int build_in_exec(Process* proc) {
	if(_strcmp(proc->path, "cd")){
		if(chdir(proc->argv[1]) < 0){
			puts("failed on chdir()\n");
			return 1;
		}
		return 0;
	}else if(_strcmp(proc->path, "exit")){
		exit(0);
	}else if(_strcmp(proc->path, "ttt")){
		printf("reply ttt\n");
		return 0;
	}else if(_strcmp(proc->path, "export")){
		char desire_key[128], desire_value[128];
		if(!_extract_env(proc->argv[1], desire_key, desire_value)){
			puts("format error");
			return 1;
		}
		int target = _search_env(environ, desire_key);
		char* new_env_str = malloc(256);// memory may leak
		_write_env(desire_key, desire_value, new_env_str);
		if(target >= 0){// replace
			environ[target] = new_env_str;
		}else{// create
			// get current environ length
			int i;
			for(i=0; environ[i]!=0; i++);
			char** new_environ = malloc((i+2)*sizeof(char*));// null pointer and new env
			// copy over
			for(i=0; environ[i]!=0; i++){
				new_environ[i] = environ[i];
			}
			new_environ[i] = new_env_str;
			new_environ[i+1] = 0;
			environ = new_environ;// memory may leak
		}
		return 0;
	}
	return -1;
}

int _strcmp(char* str, char* str2){
	int i;
	for(i=0; str[i] != 0; i++){
		if(str[i] != str2[i]){
			return 0;
		}
	}
	if(str2[i] != 0) return 0;
	else return 1;
}

void _write_env(char* key, char* value, char* output) {
	int i;
	for(i=0; key[i] != 0; i++){// filling key loop
		output[i] = key[i];
	}
	output[i++] = '=';
	int value_i = 0;
	for(;; i++, value_i++){// filling value loop
		output[i] = value[value_i];
		if(value[value_i] == 0) break;
	}
	output[i+1] = 0;
}

int _extract_env(char* env_str, char* key, char* value){
	int i;
	for(i=0; env_str[i] != '='; i++){// filling key loop
		if(key) key[i] = env_str[i];
		if(env_str[i] == 0) return 0;// failed
	}
	if(key) key[i++] = 0; 
	else i++;
	int value_i = 0;
	for(;; i++, value_i++){// filling value loop
		if(value) value[value_i] = env_str[i];
		if(env_str[i] == 0) break;
	}
	if(value) value[value_i+1] = 0;
	return 1;
}

int _search_env(char** env, char* key_to_find) {
	for(int i=0; env[i] != 0; i++){
		char key[128];
		if(_extract_env(env[i], key, 0) && _strcmp(key, key_to_find)){
			return i;
		}
	}
	return -1;
}