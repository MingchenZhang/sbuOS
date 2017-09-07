#include <stdio.h>
#include <unistd.h>

#include "include/sbush_process.h"
#include "include/sbush_exec.h"

void print_prompt() {
	int target = _search_env(environ, "PS1");
	if(target < 0){
		write(1, "sbush> ", 7);
	}else{
		char prompt[128], n[128];
		if(!_extract_env(environ[target], n, prompt)){
			puts("faile to extract PS1 env");
		}
		int i;
		for(i=0; prompt[i] != 0; i++);
		write(1, prompt, i);
	}
}

int main(int argc, char *argv[], char *envp[]){
	if(argc > 1){// script mode
		char* script_path = argv[1];
		int fd = open(script_path, 0);// read-only // might use a better way
		if(fd < 0){// no exist
			puts("encounter error while reading script file. exiting.");
			return 1;
		}
		char line[1024];
		int index = 0;
		for(;;index++){
			if(read(fd, line+index, 1) == 0){
				// EOF
				line[index] = 0;
				if(line[0] == '#' && line[1] == '!'){// shebang
				}else{
					ProcessCommand(line);
				}
				break;
			}
			if(line[index] == '\n'){
				line[index] = 0;
				if(line[0] == '#' && line[1] == '!'){// shebang
				}else{
					ProcessCommand(line);
				}
				index = -1;
				continue;
			}
		}
		close(fd);
		return 0;
	}else{
		puts("started");
		print_prompt();
		char input[1024];
		int pointer = 0;
		while(1){
		  if(read(0, input + pointer, 1) < 0) {
			  puts("unable to read. exiting...\n");
			  return 0;
		  }
		  if(*(input + pointer) == '\n'){
			  *(input + pointer) = 0;
			  pointer = 0;
			  if(!ProcessCommand(input)){
				  puts("unable to parse command.\n");
			  }
			  print_prompt();
			  continue;
		  }
		  pointer++;
		}
		return 0;
	}
}