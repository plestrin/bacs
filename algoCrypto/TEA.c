#include "TEA.h"

#define DELTA 			0x9E3779B9
#define TEA_NB_ROUND	32
#define XTEA_NB_ROUND 	32

void tea_encrypt(const uint32_t* data, const uint32_t* key, uint32_t* output){
	uint32_t y;
	uint32_t z;
	uint32_t s;
	uint32_t j;

	y = data[0];
	z = data[1];

	for (j = 0, s = DELTA; j < TEA_NB_ROUND; j++, s += DELTA){
		y += ((z << 4) + key[0]) ^ (z + s) ^ ((z >> 5) + key[1]);
		z += ((y << 4) + key[2]) ^ (y + s) ^ ((y >> 5) + key[3]);
	}

	output[0] = y;
	output[1] = z;
}

void tea_decrypt(const uint32_t* data, const uint32_t* key, uint32_t* output){
	uint32_t y;
	uint32_t z;
	uint32_t s;
	uint32_t j;

	y = data[0];
	z = data[1];

	for (j = 0, s = 0xC6EF3720; j < TEA_NB_ROUND; j++, s -= DELTA){
		z -= ((y << 4) + key[2]) ^ (y + s) ^ ((y >> 5) + key[3]);
		y -= ((z << 4) + key[0]) ^ (z + s) ^ ((z >> 5) + key[1]);
	}

	output[0] = y;
	output[1] = z;
}

void xtea_encrypt(const uint32_t* data, const uint32_t* key, uint32_t* output){
	uint32_t y;
	uint32_t z;
	uint32_t s;
	uint32_t j;

	y = data[0];
	z = data[1];

	for (j = 0, s = 0; j < XTEA_NB_ROUND; j++){
		y += (((z << 4) ^ (z >> 5)) + z) ^ (s + key[s & 3]);
		s += DELTA;
		z += (((y << 4) ^ (y >> 5)) + y) ^ (s + key[(s >> 11) & 3]);
	}

	output[0] = y;
	output[1] = z;
}

void xtea_decrypt(const uint32_t* data, const uint32_t* key, uint32_t* output){
	uint32_t y;
	uint32_t z;
	uint32_t s;
	uint32_t j;

	y = data[0];
	z = data[1];

	for (j = 0, s = DELTA * XTEA_NB_ROUND; j < XTEA_NB_ROUND; j++){
		z -= (((y << 4) ^ (y >> 5)) + y) ^ (s + key[(s >> 11) & 3]);
		s -= DELTA;
		y -= (((z << 4) ^ (z >> 5)) + z) ^ (s + key[s & 3]);
	}

	output[0] = y;
	output[1] = z;
}
