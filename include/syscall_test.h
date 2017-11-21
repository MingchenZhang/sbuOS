void sys_print(char* str);

void sys_print_num(unsigned long num);

void sys_test_kernel_wait(unsigned long second);

long sys_test_fork();

void sys_test_exit();

void sys_test_wait(long sec);

unsigned long sys_test_read(unsigned long fd, void* buffer, unsigned long size);

unsigned long sys_test_write(unsigned long fd, void* buffer, unsigned long size);