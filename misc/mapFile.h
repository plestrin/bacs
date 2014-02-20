#ifndef MAPFILE_H
#define MAPFILE_H

#include <stdint.h>

void* mapFile_map(const char* file_name, uint64_t* size);

#endif