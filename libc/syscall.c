#include <syscall.h>
#include <dirent.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <debuglib.h>

int errno = 0;

void (*handlers[64])(int);

struct dirent static_dirent;

void default_sig_handler(uint64_t sig_num){
	if(sig_num >= 64) sys_sig_return();
	if(handlers[sig_num]){
		(handlers[sig_num])((int)sig_num);
	}else if(sig_num == SIGINT || sig_num == SIGSEGV){
		exit(1);
	}
	sys_sig_return();
}
int _init(int argc, char *argv[], char *envp[]){
	sys_set_signal_handler(default_sig_handler);
	return main(argc, argv, envp);
}

DIR* opendir(const char *name){
	DIR* p = malloc(sizeof(DIR));
	// TODO: check dir existence
	int i=0;
	for(; name[i] && i<126; i++) p->path[i] = name[i];
	if(p->path[i-1] != '/') {
		p->path[i++] = '/';
	}
	p->path[i] = 0;
	p->index = 0;
	return p;
}
struct dirent *readdir(DIR *dirp){
	int64_t result = sys_list_files(dirp->path, &static_dirent, dirp->index);
	if(result<0) {
		errno = 0-result;
		return 0;
	}else{
		dirp->index++;
		return &static_dirent;
	}
}
int closedir(DIR *dirp){
	free(dirp);
	return 0;
}
int kill(pid_t pid, int sig){
	int64_t result = sys_signal(pid, (uint64_t)sig);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}
sighandler_t signal(int signum, sighandler_t handler){
	handlers[signum] = handler;
	return handler;
}
int putchar(int c){
	char buffer;
	buffer = (char)c;
	int64_t result = sys_write(1, &buffer, 1);
	if(result!=1){
		errno = 0-result;
		return -1;
	}else return 0;
}
int puts(const char *s){
	int i=0;
	for(;s[i] && i<1024; i++);
	int64_t result = sys_write(1, s, i);
	if(result!=i){
		errno = 0-result;
		return -1;
	}else return 0;
}
char *gets(char *s){
	int i=0;
	while(1){
		int64_t result = sys_read(0, s+i, 1);
		if(result!=1){
			if(i>0){
				return s;
			}else{
				return 0;
			}
		}
		if(s[i] == '\n'){
			s[i] = 0;
			return s;
		}
	}
}
void exit(int status){
	sys_exit(status);
}
int open(const char *pathname, int flags){
	int64_t result = sys_open(pathname, (uint64_t)flags);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return result;
	}
}
int close(int fd){
	int64_t result = sys_close(fd);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}
ssize_t read(int fd, void *buf, size_t count){
	int64_t result = sys_read((uint64_t)fd, buf, count);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return result;
	}
}
ssize_t write(int fd, const void *buf, size_t count){
	int64_t result = sys_write((uint64_t)fd, buf, count);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return result;
	}
}
int unlink(const char *pathname){
	int64_t result = sys_unlink(pathname);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}
int chdir(const char *path){
	int64_t result = sys_chdir(path);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}
char *getcwd(char *buf, size_t size){
	int64_t result = sys_getdir(buf, size);
	if(result<0) {
		errno = 0-result;
		return 0;
	}else{
		return buf;
	}
}
pid_t fork(){
	int64_t result = sys_fork();
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return (pid_t)result;
	}
}
pid_t wait(int *ret_status){
	int64_t result = sys_wait((int64_t)-1, ret_status);
	if(result<0) {
		errno = 0-result;
		ret_status[0] = 0;
		return -1;
	}else{
		return 0;
	}
}
int waitpid(int pid, int *ret_status){
	int64_t result = sys_wait((int64_t)pid, ret_status);
	if(result<0) {
		errno = 0-result;
		ret_status[0] = 0;
		return -1;
	}else{
		return 0;
	}
}
unsigned int sleep(unsigned int seconds){
	uint64_t seconds_ = (uint64_t)seconds;
	sys_pause(seconds_ * 1000000);
	return 0;
}
pid_t getpid(void){
	return sys_getpid();
}
pid_t getppid(void){
	return sys_getpid();
}
off_t lseek(int fd, off_t offset, int whence){
	int64_t result = sys_lseek((uint64_t) fd, offset);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return result;
	}
}
int mkdir(const char *pathname, mode_t mode){
	int64_t result = sys_mkdir(pathname);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}
int pipe(int pipefd[2]){
	int64_t result = sys_pipe(pipefd);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}
int ioctl(int fd, uint64_t op, uint64_t arg){
	int64_t result = sys_ioctl(fd, op, arg);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}
void* sbrk(unsigned long int inc){
	static void* current_brk;
	if(current_brk == 0) current_brk = (void*)sys_brk(0);
	if(inc == 0){
		return current_brk;
	}else{
		inc = ((inc-1)/4096+1)*4096;
		if(sys_brk((uint64_t)current_brk + inc) == 0){
			return current_brk;
		}else{
			return 0;
		}
	}
}
int64_t brk(unsigned long int to){
	return sys_brk(to);
}
int dup2(int oldfd, int newfd){
	int64_t result = sys_dup(oldfd, newfd);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}

int alarm(int sec){
	int64_t result = sys_alarm(sec);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}

void yield(){
	sys_yield();
}

int list_pid(int* buffer, size_t size){
	int64_t result = sys_list_pid(buffer, size);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return result;
	}
}
int pid_name(char* buffer, int pid, size_t size){
	int64_t result = sys_pid_name(buffer, pid, size);
	if(result<0) {
		errno = 0-result;
		return -1;
	}else{
		return 0;
	}
}

void _print(char* str){
	sys_print(str);
}
void _print_num(long number){
	sys_print_num(number);
}