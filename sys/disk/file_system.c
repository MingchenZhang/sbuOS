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
		int read_count = 0;
		for(; file->buffer_p_c-1 != file->buffer_c_c && size!=0; size--, read_buffer++, read_count++){
			file->buffer_c_c++;
			if(file->buffer_c_c == BUFFER_SIZE) file->buffer_c_c = 0;
			*read_buffer = file->buffer[file->buffer_c_c];
		}
		return read_count;
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
		sf_free(file);
	}
	// TODO: also free path_str
	// TODO: wake up all waiter
	return 1;
}

char* calculate_path(char* base, char* relative_path){
	uint32_t len = strlen(relative_path);
	if(relative_path[0] == '/'){
		char* new_path = sf_calloc(len+1, 1);
		memcpy(new_path, relative_path, len);
		return new_path;
	}else{
		uint32_t base_len = strlen(base);
		if(base[base_len-1] != '/'){
			char* new_path = sf_calloc(base_len+len+2, 1);
			memcpy(new_path, base, base_len);
			new_path[base_len] = '/';
			memcpy(new_path+base_len+1, relative_path, len);
			return new_path;
		}else{
			char* new_path = sf_calloc(base_len+len+1, 1);
			memcpy(new_path, base, base_len);
			memcpy(new_path+base_len, relative_path, len);
			return new_path;
		}
		
	}
}
