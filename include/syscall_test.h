void sys_print(char* str);

void sys_print_num(unsigned long num);

void sys_test_kernel_wait(unsigned long second);

long sys_test_fork();

void sys_test_exit();

void sys_test_wait(long sec);

unsigned long sys_test_read(unsigned long fd, void* buffer, unsigned long size);

unsigned long sys_test_write(unsigned long fd, void* buffer, unsigned long size);

unsigned long sys_test_open(char* path, int flags);

unsigned long sys_test_exec(char* path, char** argv, char** envp);

unsigned long sys_test_set_signal_handler(void (*handler)(unsigned long sig_num)); // 7

void sys_test_sig_return(); // 8

void sys_test_sig(unsigned long pid, unsigned long sig_num); // 9

void sys_test_alarm(unsigned int seconds);

void sys_test_dup2(int oldfd, int newfd);

void sys_test_ioctl(unsigned long fd, unsigned long op, unsigned long arg);

long sys_test_read_block(long disk_i, unsigned long LBA, void* buffer);
long sys_test_write_block(long disk_i, unsigned long LBA, void* buffer);

// ioctl op:
#define TIOCGPGRP 101
#define TIOCSPGRP 102

// signal
#define SIGINT 2
#define SIGKILL 9
#define SIGSEGV 11
#define SIGALRM 14