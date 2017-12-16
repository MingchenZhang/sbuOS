#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/defs.h>

char ** environ;

int open(const char *pathname, int flags);
#define O_CREAT    0b100
#define O_RDONLY    0b00
#define O_WRONLY    0b01

int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int unlink(const char *pathname);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);

pid_t fork();
int execve(char *filename, char *const argv[], char *const envp[]); // actual syscall
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[], char *const envp[]); // required
pid_t wait(int *status);
int waitpid(int pid, int *ret_status);

unsigned int sleep(unsigned int seconds);

pid_t getpid(void);
pid_t getppid(void);

// OPTIONAL: implement for ``on-disk r/w file system (+10 pts)''
off_t lseek(int fd, off_t offset, int whence);
//int mkdir(const char *pathname, mode_t mode);

// OPTIONAL: implement for ``signals and pipes (+10 pts)''
int pipe(int pipefd[2]);
int dup2(int oldfd, int newfd);

void* sbrk(unsigned long int inc);
int64_t brk(unsigned long int to);

#endif
