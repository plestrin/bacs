#include <stdint.h>

#define ANALYSIS_REFERENCE_IMPLEMENTATION

#include "TEA.h"

void wrapper_tea_encipher(void** input, void** output){
	uint64_t size;

	size = *(uint32_t*)input[1];
	tea_encipher((uint32_t*)input[0], size, (uint32_t*)input[2], (uint32_t*)output[0]);
}

void wrapper_tea_decipher(void** input, void** output){
	uint64_t size;

	size = *(uint32_t*)input[1];
	tea_decipher((uint32_t*)input[0], size, (uint32_t*)input[2], (uint32_t*)output[0]);
}