#ifndef _TARFS_H
#define _TARFS_H

#include <sys/defs.h>

extern char _binary_tarfs_start;
extern char _binary_tarfs_end;

struct posix_header_ustar {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag[1];
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char pad[12];
};
typedef struct posix_header_ustar posix_header_ustar;

void init_tarfs(void* start, void* end);

void scan_tarfs();

int64_t tarfs_read(char* file_path, void* buffer, int64_t size, int64_t offset);

uint64_t tarfs_find_offset(char* file_path);

#endif
