#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>

#include "../mapFile.h"
#include "../base.h"

int main(int argc, char** argv){
	void* 		file_map;
	uint64_t 	file_size;

	if (argc != 2){
		log_err("please specify one argument");
		return 0;
	}

	file_map = mapFile_map(argv[1], &file_size);
	if (file_map == NULL){
		log_err_m("unable to map file: \"%s\"", argv[1]);
	}
	else{
		log_info("mapping file is successful");
		munmap(file_map, file_size);
	}

	return 0;
}