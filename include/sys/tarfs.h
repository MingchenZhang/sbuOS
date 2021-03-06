#ifndef _TARFS_H
#define _TARFS_H

#include <sys/defs.h>
#include <sys/disk/file_system.h>

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

typedef struct tar_file_info{
	uint64_t mem_offset;
	int64_t size;
}tar_file_info;

void init_tarfs(void* start, void* end);

void scan_tarfs();

dirent_sys next_tarfs_file(char* dir_path, int next_i);

int64_t tarfs_read(char* file_path, void* buffer, int64_t size, int64_t offset);

uint64_t tarfs_find_offset(char* file_path);

tar_file_info tarfs_file_info(char* file_path);

#endif
