#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#define BUFFER_SIZE 0x1000
#define FILE_TABLE_WAITER 4

typedef struct file_table_entry file_table_entry;
typedef struct file_table_wating file_table_wating;
typedef struct file_table_entry_pair file_table_entry_pair;
typedef struct inode inode;

struct file_table_entry{
	char io_type; // 1: entry pair in, 2: entry pair out, 3: tarfs read, 4: tarfs write, 5: inode read, 6: inode write
	char* path_str;
	uint64_t offset;
	uint64_t out_to;
	uint64_t in_from;
	uint8_t buffer[BUFFER_SIZE]
	int open_count;
	file_table_wating waiters[FILE_TABLE_WAITER];
} file_table_entry;

struct file_table_wating{
	Process* waiter; // 0 if there is no waiter
	uint64_t wait_size;
	uint8_t writes_to_type; // 1: physical addr, 2: virtual addr
	void* writes_to; 
}

struct file_table_entry_pair{
	file_table_entry* in;
	file_table_entry* out;
}

struct inode {
	
} inode;

#endif