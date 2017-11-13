#include <sys/defs.h>
#include <sys/disk/file_system.h>
#include <sys/tarfs.h>
#include <sys/misc.h>

file_table_entry* file_open_read(char* path){
	uint64_t file_offset = tarfs_find_offset(path);
	if(file_offset){
		// this is a tarfs path
		file_table_entry* file = sf_calloc(sizeof(file_table_entry));
		file->io_type = 3;
		uint32_t path_len = strlen(path);
		char* path_str = sf_malloc(sizeof(char) * path_len +1);
		memcpy(path_str, path, path_len);
		file->path_str = path_str;
		file->in_from = file_offset;
		file->open_count = 1;
		return file;
	}else{
		// this is a disk path
		kprintf("file_open_read: no disk path supported yet\n");
		return 0;
	}
}

// return 0:error, 1: available now, 2: available later
// int file_read(file_table_entry* file, Process* initiator, uint64_t vir_addr, uint64_t size){
	// if(file->io_type == 3){
		// uint64_t file_addr = tarfs_find_offset(file->path_str);
		// for(uint32_t cursor=0; cursor<)
	// }else{
		// kprintf("file_read not yet supports io_type:%d\n", file->io_type);
		// return 0;
	// }
// }
