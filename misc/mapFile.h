#ifndef MAPFILE_H
#define MAPFILE_H

#include <stdint.h>
#include <sys/mman.h>

struct mappingDesc{
	void*	buffer;
	size_t 	size;
};

#define mappingDesc_free_mapping(desc) munmap((desc).buffer, (desc).size)

void* mapFile_map(const char* file_name, size_t* size);

void* mapFile_part(int file, off_t offset, size_t size, struct mappingDesc* desc);

#endif