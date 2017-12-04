#include <syscall_test.h>
#include <sys/defs.h>

#define DISK_SIZE_BOUND 0x800000
#define DISK_INDEX 1

void _memset(void* dest, unsigned char set, unsigned long size){
	uint8_t* c = dest;
	for(int i=0; i<size; i++){
		c[i] = set;
	}
}

void* memcpy(void *dest, const void *src, size_t n){
	char *pszDest =(char*)dest;
	const char *pszSource =(char*)src;
	while (n--){
		*pszDest =*pszSource;
		pszDest++; pszSource++;
	}
	return dest;
}

int main(int argc, char**argv){
	unsigned char buffer[4096];
	uint8_t disk_i = DISK_INDEX;
	// assume the disk size is known
	uint64_t disk_size = DISK_SIZE_BOUND;
	uint64_t num_block = disk_size / 0x1000;
	// write superblock
	_memset(buffer, 0, 4096);
	memcpy(buffer, "simple_fs", 9); // 0:15
	uint64_t next_availble = 1;
	memcpy(buffer+16, &next_availble, 8); // 16:23
	uint64_t last_block = num_block;
	memcpy(buffer+24, &last_block, 8); // 16:23
	if(sys_test_write_block(disk_i, 0, buffer) == -1){
		sys_print("fail to write superblock\n");
		goto failed;
	}
	// writes first file list
	_memset(buffer, 0, 4096);
	if(sys_test_write_block(disk_i, 1, buffer) == -1){
		sys_print("fail to write file list\n");
		goto failed;
	}
	sys_print("simple_fs: disk formated\n");
	sys_test_exit();
	failed:
	sys_print("simple_fs: disk format failed\n");
	sys_test_exit();
}
