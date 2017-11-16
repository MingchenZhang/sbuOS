#include <sys/defs.h>
#include <sys/disk/file_system.h>
#include <sys/tarfs.h>
#include <sys/misc.h>
#include <sys/memory/kmalloc.h>
#include <sys/kprintf.h>

file_table_entry* file_open_read(char* path){
	uint64_t file_offset = tarfs_find_offset(path);
	if(file_offset){
		// this is a tarfs path
		file_table_entry* file = sf_calloc(sizeof(file_table_entry), 1);
		file->io_type = 3;
		uint32_t path_len = strlen(path);
		char* path_str = sf_malloc(sizeof(char) * path_len +1);
		memcpy(path_str, path, path_len);
		file->path_str = path_str;
		file->in_from = file_offset;
		file->open_count = 1;
		file->buffer_p_c = 1; // where producer is going to produced at
		file->buffer_c_c = 0; // where consumer has consumed
		return file;
	}else{
		// this is a disk path
		kprintf("file_open_read: no disk path supported yet\n");
		return 0;
	}
}

// return -1:error, positive int: available now, -2: available later
// max read: BUFFER_SIZE at a time, multiple read should be implemeted at a higher level
int file_read(file_table_entry* file, Process* initiator, uint64_t size){
	if(file->io_type == 3){ // tarfs read
		tar_file_info info = tarfs_file_info(file->path_str);
		int read_count = 0;
		for(; file->offset < info.size; file->offset++, read_count++){
			if(file->buffer_p_c == file->buffer_c_c){ // if the buffer is full
				return read_count;
			}
			file->buffer[file->buffer_p_c] = ((uint8_t*)file->in_from)[file->offset];
			file->buffer_p_c++;
			if(file->buffer_p_c == BUFFER_SIZE) file->buffer_p_c = 0;
		}
		return read_count;
	}else if(file->io_type == 2){ // entry pair read
		int pc = file->buffer_p_c;
		int cc = file->buffer_c_c;
		if(pc <= cc){
			pc += 4096;
		}
		return pc-cc-1;
	}else{
		kprintf("file_read not yet supports io_type:%d\n", file->io_type);
		return 0;
	}
}

// return -1: error, positive int: available now, -2: availabel later
int file_write(file_table_entry* file, Process* initiator, uint8_t* buffer_in, uint64_t size){
	if(file->io_type == 1){ // entry pair write
		int read_count = 0;
		for(; read_count < size; read_count++){
			if(file->buffer_p_c == file->buffer_c_c){ // if the buffer is full
				return read_count;
			}
			file->buffer[file->buffer_p_c] = buffer_in[read_count];
			file->buffer_p_c++;
			if(file->buffer_p_c == BUFFER_SIZE) file->buffer_p_c = 0;
		}
		return read_count;
	}else{
		kprintf("file_read not yet supports io_type:%d\n", file->io_type);
		return 0;
	}
}
