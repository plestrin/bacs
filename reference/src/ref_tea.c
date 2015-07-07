#include <stdint.h>

#define ANALYSIS_REFERENCE_IMPLEMENTATION

#include "TEA.h"

void wrapper_tea_encrypt(void** input, void** output){
	tea_encrypt((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_tea_decrypt(void** input, void** output){
	tea_decrypt((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_xtea_encrypt(void** input, void** output){
	xtea_encrypt((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_xtea_decrypt(void** input, void** output){
	xtea_decrypt((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}