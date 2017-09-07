#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int __strcmp(char* str, char* str2){
	int i;
	for(i=0; str[i] != 0; i++){
		if(str[i] != str2[i]){
			return 0;
		}
	}
	if(str2[i] != 0) return 0;
	else return 1;
}

int __extract_env(char* env_str, char* key, char* value){
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

int __search_env(char** env, char* key_to_find) {
	for(int i=0; env[i] != 0; i++){
		char key[128];
		if(__extract_env(env[i], key, 0) && __strcmp(key, key_to_find)){
			return i;
		}
	}
	return -1;
}

char* _get_next_path(char* start_at){
	char* cursor = start_at;
	for(;;cursor++){
		if(*cursor == ':'){
			*cursor = 0;
			return cursor+1;
		}else if(*cursor == 0){
			return 0;
		}
	}
}

int execvp(char *file, char * argv[]){
	// get path
	if(file[0] == '/'){// absolute path
		int fd = open(file, 0);// read-only // might use a better way
		if(fd < 0){// no exist
			errno = 2;
			return -1;
		}else{
			close(fd);
			return execve(file, argv, environ);
		}
	}
	int target = __search_env(environ, "PATH");
	if(target < 0){// no PATH env
		int fd = open(file, 0);// read-only // might use a better way
		if(fd < 0){// no exist
			errno = 2;
			return -1;
		}else{
			close(fd);
			return execve(file, argv, environ);
		}
	}else{// has PATH env
		char path[1024];
		char path_append[512];
		if(!__extract_env(environ[target], 0, path))
			return execve(file, argv, environ);
		char* path_to_check = path;
		for(;*path_to_check != 0;){
			int i=0;
			for(; path_to_check[i] != 0 && path_to_check[i] != ':'; i++){
				path_append[i] = path_to_check[i];
			}
			if(path_to_check[i] == 0){
				break;
			}else{
				path_to_check = path_to_check + i + 1;
			}
			path_append[i++] = '/';
			for(int j=0; file[j] != 0; j++, i++){
				path_append[i] = file[j];
			}
			path_append[i++] = 0;
			// check if the file exist
			int fd = open(path_append, 0);// read-only // might use a better way
			if(fd < 0){// no exist
				continue;
			}else{
				close(fd);
				return execve(path_append, argv, environ);
			}
		}
		errno = 2;
		return -1;
	}
}