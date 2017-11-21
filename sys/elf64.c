#include <sys/kprintf.h>
#include <sys/misc.h>
#include <sys/memory/kmalloc.h>
#include <sys/elf64.h>
#include <sys/tarfs.h>
#include <sys/disk/file_system.h>

uint8_t elf64_sig[] = {0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

program_section* read_elf_tarfs(char* file_path){
	char buffer[512];
	if(tarfs_read(file_path, buffer, 0x40, 0) < 0x40){
		kprintf("DEBUG: elf header unable to load\n");
		return 0;
	}
	Elf64_Ehdr* header = (Elf64_Ehdr*)buffer;
	if(!memeq(header->e_ident, elf64_sig, 0x10)){
		kprintf("DEBUG: elf signature mismatch\n");
		return 0;
	}
	uint64_t pro_header_offset = header->e_phoff;
	program_section* first = 0;
	program_section* last = 0;
	for(int count = header->e_phnum; count>0; count--, pro_header_offset+=0x38){
		Elf64_Phdr pro_buffer;
		program_section* section = sf_malloc(sizeof(program_section));
		if(tarfs_read(file_path, &pro_buffer, 0x38, pro_header_offset) < 0x38){
			kprintf("DEBUG: elf header at %x fail to load\n", pro_header_offset);
			return 0;
		}
		if(!first) first = section;
		section->file_offset = pro_buffer.p_offset;
		section->memory_offset = pro_buffer.p_vaddr;
		section->size = pro_buffer.p_filesz;
		section->entry_point = header->e_entry;
		section->next = 0;
		if(last) last->next = section;
		last = section;
	}
	return first;
}

program_section* read_elf(file_table_entry* file){
	uint8_t buffer[512];
	file_set_offset(file, 0);
	if(file_read(file, current_process, buffer, 0x40) < 0x40){
		kprintf("DEBUG: elf header unable to load\n");
		return 0;
	}
	Elf64_Ehdr* header = (Elf64_Ehdr*)buffer;
	if(!memeq(header->e_ident, elf64_sig, 0x10)){
		kprintf("DEBUG: elf signature mismatch\n");
		return 0;
	}
	uint64_t pro_header_offset = header->e_phoff;
	program_section* first = 0;
	program_section* last = 0;
	for(int count = header->e_phnum; count>0; count--, pro_header_offset+=0x38){
		Elf64_Phdr pro_buffer;
		program_section* section = sf_malloc(sizeof(program_section));
		file_set_offset(file, pro_header_offset);
		if(file_read(file, current_process, (void*)&pro_buffer, 0x38) < 0x38){
			kprintf("DEBUG: elf header at %x fail to load\n", pro_header_offset);
			return 0;
		}
		if(!first) first = section;
		section->file_offset = pro_buffer.p_offset;
		section->memory_offset = pro_buffer.p_vaddr;
		section->size = pro_buffer.p_filesz;
		section->entry_point = header->e_entry;
		section->next = 0;
		if(last) last->next = section;
		last = section;
	}
	return first;
}
