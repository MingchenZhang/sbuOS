#include <dirent.h>
#include <unistd.h>
#include <stdio.h>


int main(int argc, char**argv){
	int fd;
	if(argc < 2){//list current
		fd = open(".", 0);
		if(fd < 0){
			puts("fail to open directory");
			return 1;
		}
	}else{//list specified
		fd = open(argv[1], 0);
		if(fd < 0){
			puts("fail to open directory");
			return 1;
		}
	}
	char buffer[4096];
	int nread;
	while((nread = getdents(fd, (struct dirent*)buffer, sizeof(buffer))) > 1){
		int off_set;
		for(off_set = 0;off_set<nread;){
			struct dirent* dir_pt = (struct dirent*)(buffer+off_set);
			char* fn = dir_pt->d_name;
			if(fn[0] == '.' && (fn[1] == 0 || (fn[1] == '.' && fn[2] == 0)));
			else puts(dir_pt->d_name);
			off_set+=dir_pt->d_reclen;
		}
	}
	close(fd);
}