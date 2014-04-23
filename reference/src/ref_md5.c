#include <stdint.h>

#define ANALYSIS_REFERENCE_IMPLEMENTATION

#include "MD5.h"

void wrapper_md5(void** input, void** output){
	uint64_t size;

	size = *(uint32_t*)input[1];
	md5((uint32_t*)input[0], size, (uint32_t*)output[0]);
}

void wrapper_md5_round1(void** input, void** output){
	md5_round1((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_md5_round2(void** input, void** output){
	md5_round2((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_md5_round3(void** input, void** output){
	md5_round3((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_md5_round4(void** input, void** output){
	md5_round4((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}