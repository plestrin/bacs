#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "mapFile.h"

void* mapFile_map(const char* file_name, size_t* size){
	int 				file;
	struct stat 		sb;
	void*				buffer;

	file = open(file_name, O_RDONLY);
	if (file == -1){
		printf("ERROR: in %s, unable to open file %s read only\n", __func__, file_name);
		perror(NULL);
		return NULL;
	}

	if (fstat(file, &sb) < 0){
		printf("ERROR: in %s, unable to read file size\n", __func__);
		close(file);
		return NULL;
	}

	*size = sb.st_size;
	buffer = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	close(file);
	if (buffer == NULL){
		printf("ERROR: in %s, unable to map file\n", __func__);
	}

	return buffer;
}