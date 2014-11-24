#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>
#include <string.h>

#include "assemblyElfLoader.h"

int32_t assembly_load_elf(struct assembly* assembly, const char* file_path){
	int 		file;
	Elf* 		elf 				= NULL;
	size_t 		shstrndx;
	Elf_Scn* 	section 			= NULL;
	GElf_Shdr 	section_header;
	char* 		name;
	Elf_Data* 	data 				= NULL;
	int32_t 	result 				= -1;

	if (elf_version(EV_CURRENT) == EV_NONE){
		printf("ERROR: in %s, ELF library initialization failed\n", __func__);
		return result;
	}

	file = open(file_path, O_RDONLY);
	if (file < 0){
		printf("ERROR: in %s, unable to open file: %s\n", __func__, file_path);
		return result;
	}

	elf = elf_begin(file, ELF_C_READ, NULL);
	if (elf == NULL){
		printf("ERROR: in %s, elf begin failed\n", __func__);
		goto exit;
	}

	if (elf_kind(elf) != ELF_K_ELF){
		printf("ERROR: in %s, %s is not an ELF object\n", __func__, file_path);
		goto exit;
	}

	if (elf_getshdrstrndx(elf, &shstrndx)){
		printf("ERROR: in %s, unable to section name offset\n", __func__);
		goto exit;
	}

	while ((section = elf_nextscn(elf, section)) != NULL){
		if (gelf_getshdr(section, &section_header) != &section_header){
			printf("ERROR: in %s, unable to retrieve section header\n", __func__);
			continue;
		}

		if ((name = elf_strptr(elf, shstrndx , section_header.sh_name)) == NULL){
			printf("ERROR: in %s, unable to retrieve section name\n", __func__);
			continue;
		}
		
		if (!strcmp(name, ".text")){
			uint32_t 			buffer_id 			= 1;
			struct asmBlock* 	buffer_block;
			uint64_t 			buffer_size_block 	= sizeof(struct asmBlockHeader) + section_header.sh_size;
			uint32_t 			offset 				= 0;

			buffer_block = (struct asmBlock*)malloc(buffer_size_block);
			if (buffer_block == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				goto exit;
			}

			while (offset < section_header.sh_size && (data = elf_getdata(section, data)) != NULL){
				memcpy(buffer_block->data + offset, data->d_buf, data->d_size);
				offset += data->d_size;
			}

			buffer_block->header.id 		= 1;
			buffer_block->header.size 		= offset;
			buffer_block->header.nb_ins 	= asmBlock_count_nb_ins(buffer_block);
			buffer_block->header.address 	= section_header.sh_addr;

			result = assembly_init(assembly, &buffer_id, sizeof(uint32_t), (uint32_t*)buffer_block, buffer_size_block, ASSEMBLYALLOCATION_MALLOC);
			break;
		}
	}

	exit:

	if (elf != NULL){
		elf_end(elf);
	}
	close(file);

	return result;
}