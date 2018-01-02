#include <syscall.h>
#include <dirent.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <debuglib.h>
#include <errno.h>



int __strcmp(const char* str, const char* str2){
	int i;
	for(i=0; str[i] != 0; i++){
		if(str[i] != str2[i]){
			return 0;
		}
	}
	if(str2[i] != 0) return 0;
	else return 1;
}

int __extract_env(const char* env_str, char* key, char* value){
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

int __search_env(char *const env[], const char* key_to_find) {
	for(int i=0; env[i] != 0; i++){
		char key[128];
		if(__extract_env(env[i], key, 0) && __strcmp(key, key_to_find)){
			return i;
		}
	}
	return -1;
}

int execvp(const char *file, char *const argv[]){
	return execvpe(file, argv, environ);
}

int execvpe(const char *file, char *const argv[], char *const env[]){
	// get path
	if(file[0] == '/'){// absolute path
		int fd = open(file, 0);// read-only // might use a better way
		if(fd < 0){// no exist
			errno = 2;
			return -1;
		}else{
			close(fd);
			return execve(file, argv, env);
		}
	}
	int target = __search_env(env, "PATH");
	if(target < 0){// no PATH env
		// _print("no path\n");
		int fd = open(file, 0);// read-only // might use a better way
		if(fd < 0){// no exist
			errno = 2;
			return -1;
		}else{
			close(fd);
			return execve(file, argv, env);
		}
	}else{// has PATH env
		// _print("found path\n");
		char path[1024];
		char path_append[512];
		if(!__extract_env(env[target], 0, path))
			return execve(file, argv, env);
		char* path_to_check = path;
		for(;path_to_check && *path_to_check != 0;){
			int i=0;
			for(; path_to_check[i] != 0 && path_to_check[i] != ':'; i++){
				path_append[i] = path_to_check[i];
			}
			if(path_to_check[i] == 0){
				path_to_check = 0; // last path
			}else{
				path_to_check = path_to_check + i + 1;
			}
			path_append[i++] = '/';
			for(int j=0; file[j] != 0; j++, i++){
				path_append[i] = file[j];
			}
			path_append[i++] = 0;
			// check if the file exist
			// _print(path_append);
			int fd = open(path_append, 0);// read-only // might use a better way
			if(fd < 0){// no exist
				continue;
			}else{
				close(fd);
				return execve(path_append, argv, env);
			}
		}
		errno = 2;
		return -1;
	}
}

int execve(const char *file, char *const argv[], char *const envp[]){
	int64_t result = sys_exec(file, argv, envp);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}