#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <sys/defs.h>
#include <dirent.h>

int64_t sys_signal(pid_t pid, uint64_t sig);

int sys_set_signal_handler(void (*handler)(uint64_t));

int64_t sys_open(const void *pathname, uint64_t flags);
#define O_CREAT    0b100
#define O_RDONLY    0b00
#define O_WRONLY    0b01

int64_t sys_close(uint64_t fd);

int64_t sys_read(uint64_t fd, const void *buf, uint64_t count);

int64_t sys_write(uint64_t fd, const void *buf, uint64_t count);

void sys_exit(int status);

int64_t sys_brk(uint64_t new_bp);

int64_t sys_unlink(const char * pathname);

int64_t sys_chdir(const char * path);

int64_t sys_getdir(const char* buffer, uint64_t size);

int64_t sys_fork();

int64_t sys_exec(const char* path, char*const argv[], char*const envp[]);

int64_t sys_wait(int64_t pid, int* status);

void sys_pause(uint64_t nano_second);

int64_t sys_getpid();

int64_t sys_getppid();

int64_t sys_lseek(uint64_t fd, uint64_t offset);

int64_t sys_mkdir(const char* path);

int64_t sys_pipe(int fd[2]);

int64_t sys_list_files(char* dir_path, struct dirent* write_to, int index);

int64_t sys_sig_return();

int64_t sys_ioctl(int fd, uint64_t op, uint64_t arg);

int sys_dup(int oldfd, int newfd);

int sys_alarm(int seconds);

void sys_print(char* str);

void sys_print_num(long number);

void sys_yield();

#endif