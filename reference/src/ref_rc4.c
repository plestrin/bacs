#include <stdint.h>

#define ANALYSIS_REFERENCE_IMPLEMENTATION

#include "RC4.h"

void wrapper_rc4(void** input, void** output){
	uint64_t input_length;
	uint8_t key_length;

	input_length = *(uint32_t*)input[1];
	key_length = *(uint32_t*)input[3];
	rc4((uint8_t*)input[0], input_length, (uint8_t*)input[2], key_length, (uint8_t*)output[0]);
}