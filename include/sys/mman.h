#ifndef SBUSH_MMAN_H
#define SBUSH_MMAN_H

#include <sys/defs.h>
void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
#endif