#include "TEA.h"

#define DELTA 		0x9E3779B9
#define NB_ROUND	32

void tea_encypher(uint32_t* data, uint64_t data_length, uint32_t* key, uint32_t* output){
	uint32_t y;
	uint32_t z;
	uint32_t s;
	uint64_t i;
	uint32_t j;
	
	for (i = 0; i < data_length / TEA_BLOCK_NB_BYTE; i++){
		y = data[2*i];
		z = data[2*i + 1];
		
		for (j = 0, s = DELTA; j < NB_ROUND; j++, s += DELTA){
			y += ((z << 4) + key[0]) ^ (z + s) ^ ((z >> 5) + key[1]);
			z += ((y << 4) + key[2]) ^ (y + s) ^ ((y >> 5) + key[3]);
		}
		
		output[2*i] = y;
		output[2*i + 1] = z;
	}
}

void tea_decipher(uint32_t* data, uint64_t data_length, uint32_t* key, uint32_t* output){
	uint32_t y;
	uint32_t z;
	uint32_t s;
	uint64_t i;
	uint32_t j;
	
	for (i = 0; i < data_length / TEA_BLOCK_NB_BYTE; i++){
		y = data[2*i];
		z = data[2*i + 1];
		
		for (j = 0, s = 0xC6EF3720; j < NB_ROUND; j++, s -= DELTA){
			z -= ((y << 4) + key[2]) ^ (y + s) ^ ((y >> 5) + key[3]);
			y -= ((z << 4) + key[0]) ^ (z + s) ^ ((z >> 5) + key[1]);
		}
		
		output[2*i] = y;
		output[2*i + 1] = z;
	}
}