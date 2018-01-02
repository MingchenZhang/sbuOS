#include <sys/defs.h>
#include <sys/disk/file_system.h>
#include <sys/tarfs.h>
#include <sys/misc.h>
#include <sys/memory/kmalloc.h>
#include <sys/kprintf.h>
#include <sys/ahci.h>

#define DEAULT_DISK_INDEX 1
uint64_t disk_block_free = 2;
uint64_t disk_size = 0x800000;

void* read_disk_block(uint8_t disk_i, Process* proc, uint64_t block){
	kernel_space_task_file.type = TASK_RW_DISK_BLOCK;
	kernel_space_task_file.param[0] = disk_i; // disk index
	kernel_space_task_file.param[1] = (uint64_t)proc;
	kernel_space_task_file.param[2] = (uint64_t)block; // LBA
	kernel_space_task_file.param[3] = (uint64_t)0;
	kernel_space_task_file.param[4] = 0;
	kernel_space_handler_wrapper();
	if(!kernel_space_task_file.ret[0]){
		return 0;
	}
	page_entry* read_to_page = (page_entry*)(uint64_t*)kernel_space_task_file.ret[1];
	current_process->on_hold = 1;
	// kprintf("DEBUG: kernel wait starts\n");
	__asm__ volatile ("int $0x81;");
	uint64_t* buffer = sf_malloc(4096);
	kernel_space_task_file.type = TASK_CP_PAGE_MALLOC;
	kernel_space_task_file.param[0] = (uint64_t)read_to_page;
	kernel_space_task_file.param[1] = (uint64_t)buffer;
	kernel_space_task_file.param[2] = 1; // page_to_malloc
	kernel_space_handler_wrapper();
	kernel_space_task_file.type = TASK_PAGE_USED_BY;
	kernel_space_task_file.param[0] = (uint64_t)read_to_page;
	kernel_space_task_file.param[1] = (uint64_t)0;
	kernel_space_handler_wrapper();
	return buffer;
}

int write_disk_block(uint8_t disk_i, Process* proc, uint64_t block, void* malloc_block){
	kernel_space_task_file.type = TASK_RW_DISK_BLOCK;
	kernel_space_task_file.param[0] = disk_i; // disk index
	kernel_space_task_file.param[1] = (uint64_t)proc;
	kernel_space_task_file.param[2] = (uint64_t)block; // LBA
	kernel_space_task_file.param[3] = (uint64_t)malloc_block;
	kernel_space_task_file.param[4] = 1;
	kernel_space_handler_wrapper();
	if(!kernel_space_task_file.ret[0]){
		return 0;
	}
	current_process->on_hold = 1;
	// kprintf("DEBUG: kernel wait starts\n");
	__asm__ volatile ("int $0x81;");
	return 1;
}

int write_superblock(uint8_t disk_i, uint64_t size, uint64_t free_starts){
	uint64_t* buffer = sf_calloc(4096, 1);
	memcpy(buffer, "simple_fs", 9); // 0:15
	buffer[2] = size;
	buffer[3] = free_starts;
	int ret = write_disk_block(disk_i, current_process, 0, buffer);
	sf_free(buffer);
	return ret;
}

inode* search_file_in_disk(char* path){
	// call kernel task to read 
	uint64_t lba_c = 1;
	while(1){
		void* readed = read_disk_block(DEAULT_DISK_INDEX, current_process, lba_c);
		if(!readed){
			sf_free(readed);
			return 0;
		}
		simple_fs_file_list* list = (simple_fs_file_list*)readed;
		int i=0;
		for(; i<31; i++){
			if((list[i].attr & (1L<<63)) && streq(list[i].name, path)){
				sf_free(readed);
				inode* file_info = sf_malloc(sizeof(inode));
				file_info->base_lba = list[i].lba;
				file_info->layer = 0; // default layer to 0
				file_info->size = list[i].attr & 0xffffffff;
				file_info->file_list_lba = lba_c;
				file_info->file_list_lba_i = i;
				return file_info;
			}
		}
		lba_c = *(uint64_t*)(list + i);
		sf_free(readed);
		if(lba_c == 0){
			return 0;
		}
	}
}

void init_file_system(){
	page_entry* w_to = find_free_page_entry();
	w_to->used_by = 2;
	ahci_read(find_port(DEAULT_DISK_INDEX), 0, 0, 8, w_to->base);
	uint64_t* info = (uint64_t*)(w_to->base);
	if(!streq((char*)info, "simple_fs")){
		kprintf("warning: file system not formated, formating...\n");
		int result = ahci_write(find_port(DEAULT_DISK_INDEX), 0, 0, 8, w_to->base);
		if(!result){
			kprintf("error: file system cannot be formated.\n");
			w_to->used_by = 0;
			disk_size = 0;
		}
		return;
	}
	disk_size = info[2];
	disk_block_free = info[3];
	return;
}

int create_file_in_disk(char* path){
	uint64_t lba_c = 1;
	while(1){
		void* readed = read_disk_block(DEAULT_DISK_INDEX, current_process, lba_c);
		if(!readed){
			sf_free(readed);
			return 0;
		}
		simple_fs_file_list* list = (simple_fs_file_list*)readed;
		int i=0;
		for(; i<31; i++){
			if(!(list[i].attr & (1L<<63))){
				list[i].attr = 0x8000000000000000; // just set the present bit
				memset(list[i].name, 0, 112);
				for(int j=0; path[j]; j++) list[i].name[j] = path[j];
				list[i].lba = disk_block_free;
				if(!write_disk_block(DEAULT_DISK_INDEX, current_process, lba_c, readed)){
					kprintf("warning: fail to write file list\n");
					sf_free(readed);
					return 0;
				}
				// allocate one block for this file, by writing to the superblock
				disk_block_free++;
				if(!write_superblock(DEAULT_DISK_INDEX, disk_size, disk_block_free)){
					kprintf("warning: fail to write superblock\n");
					sf_free(readed);
					return 0;
				}
				sf_free(readed);
				return 1;
			}
		}
		assert(0, "assert: # of file exceed 31\n"); // TODO: solve 31 limit
		sf_free(readed);
		if(lba_c == 0){
			return 0;
		}
	}
}

dirent_sys directory_compare(char* file, char* dir_path){
	char* sub_name = file;
	if(sub_name[0] == 0) goto no_match;
	int i = 0;
	for(; dir_path[i]; i++){
		if(dir_path[i] != sub_name[i]) goto no_match;
	}
	int file_name_start = i;
	for(; sub_name[i]; i++){
		if(sub_name[i] == '/' && sub_name[i+1] != 0) goto no_match;
	}
	char* filename = sf_malloc(i-file_name_start+4);
	memcpy(filename, sub_name + file_name_start, i-file_name_start+1);
	dirent_sys ret;
	ret.result = 1;
	ret.inode = 0;
	ret.name = filename;
	return ret;
	
	no_match:
	ret.result = 0;
	return ret;
}

dirent_sys list_next_file(char* dir_path, int next_i){
	uint64_t file_offset = tarfs_find_offset(dir_path);
	if(file_offset || (dir_path[0] == '/' && dir_path[1] == 0)){
		// this is a tarfs dir path
		dirent_sys dirent = next_tarfs_file(dir_path, next_i);
		return dirent;
	}else{
		uint64_t lba_c = 1;
		uint64_t counter = 0;
		while(1){
			void* readed = read_disk_block(DEAULT_DISK_INDEX, current_process, lba_c);
			if(!readed){
				sf_free(readed);
				dirent_sys ret;
				ret.result = 0;
				return ret;
			}
			simple_fs_file_list* list = (simple_fs_file_list*)readed;
			int i=0;
			for(; i<31; i++){
				if((list[i].attr & (1L<<63))){
					dirent_sys result = directory_compare(list[i].name, dir_path);
					if(!result.result){
						continue;
					}
					if(counter == next_i){
						return result;
					}
					counter++;
					sf_free(result.name);
				}
			}
			lba_c = *(uint64_t*)(list + i);
			sf_free(readed);
			if(lba_c == 0){
				dirent_sys ret;
				ret.result = 0;
				return ret;
			}
		}
		
	}
	dirent_sys ret;
	return ret;
}

file_table_entry* file_open_read(char* path){
	uint64_t file_offset = tarfs_find_offset(path);
	if(file_offset){
		// this is a tarfs path
		file_table_entry* file = sf_calloc(sizeof(file_table_entry), 1);
		file->io_type = 3;
		uint32_t path_len = strlen(path);
		char* path_str = sf_malloc(sizeof(char) * path_len +1);
		memcpy(path_str, path, path_len+1);
		file->path_str = path_str;
		file->in_from = file_offset;
		file->open_count = 1;
		file->buffer_p_c = 1; // where producer is going to produced at
		file->buffer_c_c = 0; // where consumer has consumed
		return file;
	}else{
		// this might be a disk path
		inode* file_info = search_file_in_disk(path);
		if(!file_info){
			return 0;
		}
		file_table_entry* file = sf_calloc(sizeof(file_table_entry), 1);
		file->io_type = 5;
		uint32_t path_len = strlen(path);
		char* path_str = sf_malloc(sizeof(char) * path_len +1);
		memcpy(path_str, path, path_len);
		file->path_str = path_str;
		file->in_from = (uint64_t)file_info;
		file->open_count = 1;
		file->buffer_p_c = 1; // where producer is going to produced at
		file->buffer_c_c = 0; // where consumer has consumed
		return file;
	}
}

file_table_entry* file_open_write(char* path){
	uint64_t file_offset = tarfs_find_offset(path);
	if(file_offset){
		// this is a tarfs path
		kprintf("warning: attemp to write to tarfs file");
		return 0;
	}else{
		// this might be a disk path
		inode* file_info = search_file_in_disk(path);
		if(!file_info){
			return 0;
		}
		file_table_entry* file = sf_calloc(sizeof(file_table_entry), 1);
		file->io_type = 6;
		uint32_t path_len = strlen(path);
		char* path_str = sf_malloc(sizeof(char) * path_len +1);
		memcpy(path_str, path, path_len);
		file->path_str = path_str;
		file->out_to = (uint64_t)file_info;
		file->open_count = 1;
		file->buffer_p_c = 1; // where producer is going to produced at
		file->buffer_c_c = 0; // where consumer has consumed
		file->offset = file_info->size;
		return file;
	}
}

void generate_entry_pair(file_table_entry** assign_to){
	file_table_entry* file_in = sf_calloc(sizeof(file_table_entry), 1);
	file_in->io_type = 1;
	file_in->path_str = "entry_pair_in";
	file_in->open_count = 1;
	file_in->buffer_p_c = 1; // where producer is going to produced at
	file_in->buffer_c_c = 0; // where consumer has consumed
	file_table_entry* file_out = sf_calloc(sizeof(file_table_entry), 1);
	file_out->io_type = 2;
	file_out->path_str = "entry_pair_out";
	file_out->open_count = 1;
	file_out->buffer_p_c = 1; // where producer is going to produced at
	file_out->buffer_c_c = 0; // where consumer has consumed
	file_out->in_from = (uint64_t)file_in;
	file_in->out_to = (uint64_t)file_out;
	assign_to[0] = file_out;
	assign_to[1] = file_in;
}

// return -1:error, positive int: available now, -2: available later
// max read: BUFFER_SIZE at a time, multiple read should be implemeted at a higher level
int file_read(file_table_entry* file, Process* initiator, uint8_t* read_buffer, uint64_t size){
	if(file->io_type == 3){ // tarfs read
		int64_t readed = tarfs_read(file->path_str, read_buffer, size, file->offset);
		file->offset += readed;
		return readed;
	}else if(file->io_type == 2){ // entry pair read
		if(((file_table_entry*)file->in_from)->open_count <= 0 && file->buffer_p_c-1 == file->buffer_c_c){
			return -1;
		}
		int read_count = 0;
		for(; file->buffer_p_c-1 != file->buffer_c_c && size!=0; size--, read_buffer++, read_count++){
			file->buffer_c_c++;
			if(file->buffer_c_c == BUFFER_SIZE) file->buffer_c_c = 0;
			*read_buffer = file->buffer[file->buffer_c_c];
		}
		return read_count;
	}else if(file->io_type == 5){
		inode* file_info = (inode*)file->in_from;
		assert(file_info->layer == 0, "file_info->layer larger than zero\n");
		void* readed = read_disk_block(DEAULT_DISK_INDEX, current_process, file_info->base_lba);
		uint64_t to_read = math_min(size, file_info->size-file->offset, (uint64_t)-1);
		if(to_read == 0) return -1;
		memcpy(read_buffer, readed + file->offset , to_read);
		file->offset += to_read;
		return to_read;
	}else{
		kprintf("file_read not yet supports io_type:%d\n", file->io_type);
		return -1;
	}
}

// return -1: error, positive int: available now, -2: availabel later
int file_write(file_table_entry* file, Process* initiator, uint8_t* buffer_in, uint64_t size){
	if(file->io_type == 1){ // entry pair write
		file_table_entry* out_to = (file_table_entry*)file->out_to;
		assert(out_to != 0, "assert: file_write: file->out_to is null\n");
		int read_count = 0;
		for(; read_count < size; read_count++){
			if(out_to->buffer_p_c == out_to->buffer_c_c){ // if the buffer is full
				return read_count;
			}
			out_to->buffer[out_to->buffer_p_c] = buffer_in[read_count];
			out_to->buffer_p_c++;
			if(out_to->buffer_p_c == BUFFER_SIZE) out_to->buffer_p_c = 0;
		}
		// wake up all waiter on read side
		file_table_waiting* cursor = ((file_table_entry*)(file->out_to))->first_waiters;
		while(cursor){
			cursor->waiter->on_hold = 0;
			file_table_waiting* volatile to_be_free = cursor;
			cursor = cursor->next;
			to_be_free->next = 0;
			sf_free(to_be_free);
		}
		((file_table_entry*)(file->out_to))->first_waiters = 0;
		return read_count;
	}else if(file->io_type == 6){
		// read the block first
		inode* file_info = (inode*)file->out_to;
		assert(file_info->layer == 0, "file_info->layer larger than zero\n");
		void* readed = read_disk_block(DEAULT_DISK_INDEX, current_process, file_info->base_lba);
		uint8_t* readed_ = readed;
		int written = 0;
		if(file->offset > 4096){
			file->offset = 4096;
		}
		if(file->offset + size > 4096){ // make sure the file don't exceed the size limit
			size = 4096-file->offset;
		}
		for(uint64_t c = 0; c<size; c++){
			(readed_+file->offset)[c] = buffer_in[c];
			written++;
		}
		if(write_disk_block(DEAULT_DISK_INDEX, current_process, file_info->base_lba, readed)){
		}else{
			return -1;
		}
		// write file list to increase the size
		if(file_info->size < written + file->offset) file_info->size = written + file->offset;
		void* readed_l = read_disk_block(DEAULT_DISK_INDEX, current_process, file_info->file_list_lba);
		simple_fs_file_list* list = readed_l;
		list[file_info->file_list_lba_i].attr = ((list[file_info->file_list_lba_i].attr) & 0xffffffff00000000)|(file_info->size & 0xffffffff);
		if(!write_disk_block(DEAULT_DISK_INDEX, current_process, file_info->file_list_lba, readed_l)){
			sf_free(readed_l);
			return -1;
		}
		sf_free(readed_l);
		return written;
	}else{
		kprintf("file_read not yet supports io_type:%d\n", file->io_type);
		return -1;
	}
}

void file_set_offset(file_table_entry* file, uint64_t offset){
	file->offset = offset;
}

int file_close(file_table_entry* file){
	if(--file->open_count <= 0){
		// not freeing it, because i'm sucks at it.
		// sf_free(file);
	}
	// TODO: also free path_str
	// TODO: wake up all waiter
	return 1;
}

static void str_collapse(char* dest, char* src){
    int i=0;
    for(; src[i]; i++){
        dest[i] = src[i];
    }
    dest[i] = 0;
}

static char* shorten_path(char* path){
	// deal with dot path
	for(int i=0; path[i]; i++){
		if(path[i] == '.' && (path[i+1] == '/' || path[i+1] == 0)){
		    // there is a 'this' directory path
		    if(path[i+1] == 0){
		        path[i] = 0;
		    }
		    str_collapse(path+i, path+i+2);
		    
		}
		if(path[i] == '.' && path[i+1] == '.' && (path[i+2] == '/' || path[i+2] == 0)){
		    // go search for previous dir
		    int j=i-2;
		    for(; j>=0; j--){
		        if(path[j] == '/') break;
		    }
		    if(j<0){
		        // no previous dir
		        if( path[i+2] != 0) str_collapse(path+i, path+i+3);
		        else path[i] = 0;
		    }else if(path[j] == '/'){
		        if( path[i+2] != 0) str_collapse(path+j+1, path+i+3);
		        else path[j+1] = 0;
		        i = j;
		    }
		}
	}
	return path;
}

char* calculate_path(char* base, char* relative_path){
	uint32_t len = strlen(relative_path);
	if(relative_path[0] == '/'){
		char* new_path = sf_calloc(len+1, 1);
		memcpy(new_path, relative_path, len);
		shorten_path(new_path);
		return new_path;
	}else{
		uint32_t base_len = strlen(base);
		if(base[base_len-1] != '/'){
			char* new_path = sf_calloc(base_len+len+2, 1);
			memcpy(new_path, base, base_len);
			new_path[base_len] = '/';
			memcpy(new_path+base_len+1, relative_path, len);
			shorten_path(new_path);
			return new_path;
		}else{
			char* new_path = sf_calloc(base_len+len+1, 1);
			memcpy(new_path, base, base_len);
			memcpy(new_path+base_len, relative_path, len);
			shorten_path(new_path);
			return new_path;
		}
		
	}
}

