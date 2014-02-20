#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>

#include "../mapFile.h"

int main(int argc, char** argv){
	void* 		file_map;
	uint64_t 	file_size;

	if (argc != 2){
		printf("ERROR: in %s, please specify one argument\n", __func__);
		return 0;
	}

	file_map = mapFile_map(argv[1], &file_size);
	if (file_map == NULL){
		printf("ERROR: in %s, unable to map file\n", __func__);
	}
	else{
		printf("Mapping file is successful!\n");
		munmap(file_map, file_size);
	}

	return 0;
}