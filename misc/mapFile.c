#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "mapFile.h"
#include "base.h"

void* mapFile_map(const char* file_name, size_t* size){
	int 				file;
	struct stat 		sb;
	void*				buffer;

	file = open(file_name, O_RDONLY);
	if (file == -1){
		log_err_m("unable to open file %s read only", file_name);
		return NULL;
	}

	if (fstat(file, &sb) < 0){
		log_err("unable to read file size");
		close(file);
		return NULL;
	}

	*size = sb.st_size;
	buffer = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	close(file);
	if (buffer == MAP_FAILED){
		log_err("unable to map file");
		return NULL;
	}

	return buffer;
}

void* mapFile_part(int file, off_t offset, size_t size, struct mappingDesc* desc){
	long 	page_size;
	size_t 	align;

	if ((page_size = sysconf(_SC_PAGE_SIZE)) == -1){
		log_err("unable get PAGE_SIZE value");
		return NULL;
	}

	align = offset % page_size;
	desc->size = size + align;
	desc->buffer = mmap(NULL, size + align, PROT_READ, MAP_PRIVATE, file, offset - align);
	if (desc->buffer == MAP_FAILED){
		log_err("unable to map file");
		return NULL;
	}

	return (char*)desc->buffer + align;
}