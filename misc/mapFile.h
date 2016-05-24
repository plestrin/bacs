#ifndef MAPFILE_H
#define MAPFILE_H

#ifndef WIN32
#include <stdint.h>
#include <sys/mman.h>
#endif

struct mappingDesc{
	void*	buffer;
	size_t 	size;
};

#ifndef WIN32

#define mappingDesc_free_mapping(desc) munmap((desc).buffer, (desc).size)

void* mapFile_map(const char* file_name, size_t* size);

void* mapFile_part(int file, off_t offset, size_t size, struct mappingDesc* desc);

#endif

#endif