#include <sys/tarfs.h>
#include <sys/kprintf.h>
#include <sys/misc.h>
#include <sys/memory/kmalloc.h>

void* tarfs_start;
void* tarfs_end;

static long long read_oct(char* str){
    char* c;
    for(c=str; *c; c++);
    long long len = c-str-1;
    long long accu = 0;
    long long index = 0;
    for(;len >= 0;len--, index++){
        accu += (str[len]-'0') << (index*3);
    }
    return accu;
}

void init_tarfs(void* start, void* end){
	tarfs_start = start;
	tarfs_end = end;
}

void scan_tarfs(){
	void* cursor = tarfs_start;
	kprintf("dumping tarfs names\n");
	while((void*)cursor<tarfs_end){
		char* name = ((posix_header_ustar*)cursor)->name;
		if(name[0] == 0) break;
		kprintf("%s ", name);
		cursor += ((((read_oct(((posix_header_ustar*)cursor)->size)-1)>>9)+1)<<9) + 512;
	}
	kprintf("\ndumping finished\n");
}

dirent_sys next_tarfs_file(char* dir_path, int next_i){
	dir_path++;
	void* cursor = tarfs_start;
	while((void*)cursor<tarfs_end){
		char* name = ((posix_header_ustar*)cursor)->name;
		if(name[0] == 0) break;
		if(streq(name, dir_path) || dir_path[0] == 0){
			// found the directory
			cursor += ((((read_oct(((posix_header_ustar*)cursor)->size)-1)>>9)+1)<<9) + 512;
			int counter = 0;
			while((void*)cursor<tarfs_end){
				char* sub_name = ((posix_header_ustar*)cursor)->name;
				if(sub_name[0] == 0) break;
				int i = 0;
				for(; dir_path[i]; i++){
					if(dir_path[i] != sub_name[i]) goto next_file;
				}
				int file_name_start = i;
				for(; sub_name[i]; i++){
					if(sub_name[i] == '/' && sub_name[i+1] != 0) goto next_file;
				}
				if(counter == next_i){
					char* filename = sf_malloc(i-file_name_start+4);
					memcpy(filename, sub_name + file_name_start, i-file_name_start+1);
					dirent_sys ret;
					ret.result = 1;
					ret.inode = 0;
					ret.name = filename;
					return ret;
				}else{
					counter++;
				}
				next_file:
				cursor += ((((read_oct(((posix_header_ustar*)cursor)->size)-1)>>9)+1)<<9) + 512;
			}
			// no more files
			dirent_sys ret;
			ret.result = 0;
			return ret;
		}
		cursor += ((((read_oct(((posix_header_ustar*)cursor)->size)-1)>>9)+1)<<9) + 512;
	}
	dirent_sys ret;
	ret.result = 0;
	return ret;
}

int64_t tarfs_read(char* file_path, void* buffer, int64_t size, int64_t offset){
	void* cursor = tarfs_start;
	while((void*)cursor<tarfs_end){
		char* name = ((posix_header_ustar*)cursor)->name;
		if(name[0] == 0) break;
		int64_t file_size = read_oct(((posix_header_ustar*)cursor)->size);
		if(streq(((posix_header_ustar*)cursor)->name, file_path+1)){
			int64_t size_to_copy = ((file_size-offset)>size)?size:(file_size-offset);
			if(size_to_copy < 0) return -1;
			cursor += 512;
			memcpy(buffer, cursor+offset, size_to_copy);
			return size_to_copy;
		}
		cursor += ((((file_size-1)>>9)+1)<<9) + 512;
	}
	return -1;
}

uint64_t tarfs_find_offset(char* file_path){
	void* cursor = tarfs_start;
	while((void*)cursor<tarfs_end){
		char* name = ((posix_header_ustar*)cursor)->name;
		if(name[0] == 0) break;
		int64_t file_size = read_oct(((posix_header_ustar*)cursor)->size);
		if(streq(((posix_header_ustar*)cursor)->name, file_path+1)){
			cursor += 512;
			return (uint64_t)cursor;
		}
		cursor += ((((file_size-1)>>9)+1)<<9) + 512;
	}
	return 0;
}

tar_file_info tarfs_file_info(char* file_path){
	tar_file_info info;
	void* cursor = tarfs_start;
	while((void*)cursor<tarfs_end){
		char* name = ((posix_header_ustar*)cursor)->name;
		if(name[0] == 0) break;
		int64_t file_size = read_oct(((posix_header_ustar*)cursor)->size);
		if(streq(((posix_header_ustar*)cursor)->name, file_path+1)){
			info.mem_offset = (uint64_t)(cursor + 512);
			info.size = file_size;
			return info;
		}
		cursor += ((((file_size-1)>>9)+1)<<9) + 512;
	}
	info.mem_offset = 0;
	return info;
}