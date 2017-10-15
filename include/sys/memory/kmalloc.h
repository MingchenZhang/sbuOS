#ifndef MALLOC_H
#define MALLOC_H

#include <sys/defs.h>

#define ALLOC_SIZE_BITS 4
#define BLOCK_SIZE_BITS 28
#define REQST_SIZE_BITS 32
#define PAGE_SIZE 4096

struct __attribute__((__packed__)) sf_header{
    uint64_t alloc : ALLOC_SIZE_BITS;
    uint64_t block_size : BLOCK_SIZE_BITS;
    uint64_t requested_size : REQST_SIZE_BITS;
};
typedef struct sf_header sf_header;

struct __attribute__((__packed__)) sf_free_header {
    sf_header header;
    struct sf_free_header *next;
    struct sf_free_header *prev;
};
typedef struct sf_free_header sf_free_header;

struct __attribute__((__packed__)) sf_footer {
    uint64_t alloc : ALLOC_SIZE_BITS;
    uint64_t block_size : BLOCK_SIZE_BITS;
    /* Other 32-bits are unused */
};
typedef struct sf_footer sf_footer;

void* sf_malloc(size_t size);
void sf_free(void *ptr);
void* sf_realloc(void *ptr, size_t size);
void* sf_calloc(size_t nmemb, size_t size);

#endif