

#include <sys/defs.h>
#include <sys/disk/file_system.h>

#ifndef _ELF64_H
#define _ELF64_H

#define EI_NIDENT 16

#define Elf64_Addr   uint64_t
#define Elf64_Half   uint16_t
#define Elf64_Lword  uint64_t
#define Elf64_Off    uint64_t
#define Elf64_Sword  uint32_t
#define Elf64_Sxword uint64_t
#define Elf64_Word   uint32_t
#define Elf64_Xword  uint64_t

typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half    e_type;
  Elf64_Half    e_machine;
  Elf64_Word    e_version;
  Elf64_Addr    e_entry;
  Elf64_Off     e_phoff;
  Elf64_Off     e_shoff;
  Elf64_Word    e_flags;
  Elf64_Half    e_ehsize;
  Elf64_Half    e_phentsize;
  Elf64_Half    e_phnum;
  Elf64_Half    e_shentsize;
  Elf64_Half    e_shnum;
  Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

typedef struct {
  Elf64_Word    p_type;
  Elf64_Word    p_flags;
  Elf64_Off     p_offset;
  Elf64_Addr    p_vaddr;
  Elf64_Addr    p_paddr;
  Elf64_Xword   p_filesz;
  Elf64_Xword   p_memsz;
  Elf64_Xword   p_align;
} Elf64_Phdr;

typedef struct program_section{
	uint64_t file_offset;
	uint64_t memory_offset;
	uint64_t size;
	uint64_t mem_size;
	uint64_t entry_point;
	struct program_section* next;
} program_section;

program_section* read_elf_tarfs(char* file_path);
program_section* read_elf(file_table_entry* file);

#endif
