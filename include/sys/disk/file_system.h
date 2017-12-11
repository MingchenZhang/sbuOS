#include <sys/thread/kthread.h>

#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

struct file_table_entry;

#define BUFFER_SIZE 0x1000
#define FILE_TABLE_WAITER 4

typedef struct file_table_entry file_table_entry;
typedef struct file_table_waiting file_table_waiting;
typedef struct file_table_entry_pair file_table_entry_pair;
typedef struct inode inode;
typedef struct simple_fs_file_list simple_fs_file_list;

struct file_table_waiting{
	Process* waiter; // 0 if there is no waiter
	// uint8_t writes_to_type; // 1: physical addr, 2: virtual addr
	file_table_waiting* next;
};

struct file_table_entry{
	char io_type; // 1: entry pair in, 2: entry pair out, 3: tarfs read, 4: tarfs write, 5: inode read, 6: inode write
	char* path_str;
	uint64_t offset;
	uint64_t out_to;
	uint64_t in_from;
	uint8_t buffer[BUFFER_SIZE];
	int buffer_c_c; // consumer cursor
	int buffer_p_c; // producer cursor
	int open_count;
	file_table_waiting* first_waiters;
};

struct file_table_entry_pair{
	file_table_entry* in;
	file_table_entry* out;
};

struct inode {
	uint64_t base_lba;
	uint8_t layer;
	uint64_t size;
	uint64_t file_list_lba;
	int file_list_lba_i;
};

struct simple_fs_file_list {
	char name[112];
	uint64_t attr;
	uint64_t lba;
} __attribute__((packed)); 

void init_file_system();
int create_file_in_disk(char* path);
inode* search_file_in_disk(char* path);
file_table_entry* file_open_read(char* path);
file_table_entry* file_open_write(char* path);
void generate_entry_pair(file_table_entry** assign_to);
int file_read(file_table_entry* file, Process* initiator, uint8_t* read_buffer, uint64_t size);
int file_write(file_table_entry* file, Process* initiator, uint8_t* buffer_in, uint64_t size);
void file_set_offset(file_table_entry* file, uint64_t offset);
int file_close(file_table_entry* file);

char* calculate_path(char* base, char* relative_path);

#endif