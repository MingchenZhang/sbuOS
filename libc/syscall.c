#include <sys/defs.h>
#include <dirent.h>

void sys_exit(int status);
ssize_t sys_read(int fd, void *buf, size_t count);
ssize_t sys_write(int fd, const void *buf, size_t count);
int sys_close(int fd);
int sys_pipe(int pipefd[2]);
int sys_dup2(int oldfd, int newfd);
pid_t sys_fork();
int sys_execve(const char *filename, char *const argv[], char *const envp[]);
int sys_waitpid(pid_t pid, int *stat_loc, int options);
void* sys_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
int sys_chdir(const char *path);
long int sys_brk(void* addr);
int sys_open(char* path, int flags, unsigned short mode);
int sys_old_readdir(unsigned int fd, struct dirent *dirp, unsigned int count);
int sys_getdents(unsigned int fd, struct dirent *dirp, unsigned int count);

int errno = 0;

void exit(int status){
	sys_exit(status);
}

ssize_t read(int fd, void *buf, size_t count){
	ssize_t ret = sys_read(fd, buf, count);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
	ssize_t ret = sys_write(fd, buf, count);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

int close(int fd) {
	int ret = sys_close(fd);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

int pipe(int pipefd[2]){
	int ret = sys_pipe(pipefd);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

int dup2(int oldfd, int newfd){
	int ret = sys_dup2(oldfd, newfd);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

pid_t fork() {
	int ret = sys_fork();
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
	int ret = sys_execve(filename, argv, envp);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

int waitpid(pid_t pid, int *stat_loc, int options) {
	int ret = sys_waitpid(pid, stat_loc, options);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off) {
	void* ret = sys_mmap(addr, len, prot, flags, fildes, off);
	if(((long)ret) < 0 && ((long)ret) > -35){
		errno = -(long)ret;
		return (void*)-1;
	}
	return ret;
}

int chdir(const char *path){
	int ret = sys_chdir(path);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

void* sbrk(unsigned long int inc){
	void* ret;
	if(inc == 0){
		return (void*)sys_brk(0);
	}else{
		void* cur_brk = (void*)sys_brk(0);
		if((ret = (void*)sys_brk(cur_brk + inc))<0){
			errno = (int)(long int)ret;
			return (void*)-1;
		}else{
			return cur_brk;
		}
	}
}

int open(char *pathname, int flags){
	int ret = sys_open(pathname, flags, 0);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

int readdir_fd(unsigned int fd, struct dirent *dirp, 
            unsigned int count){
	int ret = sys_old_readdir(fd, dirp, count);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

int getdents(unsigned int fd, struct dirent *dirp, 
            unsigned int count){
	int ret = sys_getdents(fd, dirp, count);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

