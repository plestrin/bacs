#ifndef ASSEMBLYELFLOADER_H
#define ASSEMBLYELFLOADER_H

#include <stdint.h>

#include "assembly.h"

int32_t assembly_load_elf(struct assembly* assembly, const char* file_path);

#endif
