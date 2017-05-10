#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mode.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

static inline void xor(const void* input1, const void* input2, void* output, uint32_t size){
	uint32_t i;

	for (i = 0; i < size / 4; i++){
		*((uint32_t*)(output) + i) = *((const uint32_t*)(input1) + i) ^ *((const uint32_t*)(input2) + i);
	}
	switch (size % 4){
		case 3 : *((uint8_t*)(output) + 4*i + 0) = *((const uint8_t*)(input1) + 4*i + 0) ^ *((const uint8_t*)(input2) + 4*i + 0);
		case 2 : *((uint8_t*)(output) + 4*i + 1) = *((const uint8_t*)(input1) + 4*i + 1) ^ *((const uint8_t*)(input2) + 4*i + 1);
		case 1 : *((uint8_t*)(output) + 4*i + 2) = *((const uint8_t*)(input1) + 4*i + 2) ^ *((const uint8_t*)(input2) + 4*i + 2);
	}
}

void mode_enc_ecb(blockCipher encrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key){
	size_t i;

	for (i = 0; i < size / block_size; i++){
		encrypt(input + i * block_size, key, output + i * block_size);
	}
}

void mode_dec_ecb(blockCipher decrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key){
	size_t i;

	for (i = 0; i < size / block_size; i++){
		decrypt(input + i * block_size, key, output + i * block_size);
	}
}

void mode_enc_cbc(blockCipher encrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key, const uint8_t* iv){
	size_t i;

	for (i = 0; i < size / block_size; i++){
		if (!i){
			xor(iv, input, output, block_size);
		}
		else{
			xor(output + (i - 1) * block_size, input + i * block_size, output + i * block_size, block_size);
		}
		encrypt(output + i * block_size, key, output + i * block_size);
	}
}

void mode_dec_cbc(blockCipher decrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key, const uint8_t* iv){
	size_t i;

	for (i = 0; i < size / block_size; i++){
		decrypt(input + i * block_size, key, output + i * block_size);
		if (!i){
			xor(iv, output, output, block_size);
		}
		else{
			xor(input + (i - 1) * block_size, output + i * block_size, output + i * block_size, block_size);
		}
	}
}

void mode_enc_ofb(blockCipher encrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key, const uint8_t* iv){
	size_t i;

	for (i = 0; i < size / block_size; i++){
		if (!i){
			encrypt(iv, key, output);
		}
		else{
			encrypt(output + (i - 1) * block_size, key, output + i * block_size);
		}
	}

	xor(input, output, output, size);
}

void mode_dec_ofb(blockCipher encrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key, const uint8_t* iv){
	size_t i;

	for (i = 0; i < size / block_size; i++){
		if (!i){
			encrypt(iv, key, output);
		}
		else{
			encrypt(output + (i - 1) * block_size, key, output + i * block_size);
		}
	}

	xor(input, output, output, size);
}

void mode_enc_cfb(blockCipher encrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key, const uint8_t* iv){
	size_t i;

	for (i = 0; i < size / block_size; i++){
		if (!i){
			encrypt(iv, key, output);
		}
		else{
			encrypt(output + (i - 1) * block_size, key, output + i * block_size);
		}
		xor(input + i * block_size, output + i * block_size, output + i * block_size, block_size);
	}
}

void mode_dec_cfb(blockCipher encrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key, const uint8_t* iv){
	size_t i;

	for (i = 0; i < size / block_size; i++){
		if (!i){
			encrypt(iv, key, output);
		}
		else{
			encrypt(input + (i - 1) * block_size, key, output + i * block_size);
		}
		xor(input + i * block_size, output + i * block_size, output + i * block_size, block_size);
	}
}

void mode_enc_ctr(blockCipher encrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key, const uint8_t* iv){
	size_t 	i;
	uint64_t local_iv[MAX_BLOCK_SIZE / 8];

	if (block_size > MAX_BLOCK_SIZE){
		printf("ERROR: in %s, block size (%u) is larger than MAX_BLOCK_SIZE (%u)\n", __func__, block_size, MAX_BLOCK_SIZE);
		return;
	}

	memcpy(local_iv, iv, block_size);

	for (i = 0; i < size / block_size; i++){
		encrypt((uint8_t*)local_iv, key, output + i * block_size);
		if (local_iv[0] == 0xffffffffffffffffULL){
			local_iv[1] = local_iv[1] + 1;
			local_iv[0] = 0;
		}
		else{
			local_iv[0] = local_iv[0] + 1;
		}
	}

	xor(input, output, output, size);
}

void mode_dec_ctr(blockCipher encrypt, uint32_t block_size, const uint8_t* input, uint8_t* output, size_t size, const void* key, const uint8_t* iv){
	size_t 	i;
	uint64_t local_iv[MAX_BLOCK_SIZE / 8];

	if (block_size > MAX_BLOCK_SIZE){
		printf("ERROR: in %s, block size (%u) is larger than MAX_BLOCK_SIZE (%u)\n", __func__, block_size, MAX_BLOCK_SIZE);
		return;
	}

	memcpy(local_iv, iv, block_size);

	for (i = 0; i < size / block_size; i++){
		encrypt((uint8_t*)local_iv, key, output + i * block_size);
		if (local_iv[0] == 0xffffffffffffffffULL){
			local_iv[1] = local_iv[1] + 1;
			local_iv[0] = 0;
		}
		else{
			local_iv[0] = local_iv[0] + 1;
		}
	}

	xor(input, output, output, size);
}

#define IPAD 0x36
#define OPAD 0x5c

int32_t hmac(const struct hash* hash, const uint8_t* input, uint8_t* output, size_t size, const uint8_t* key, size_t key_size){
	uint8_t* block;
	uint32_t i;

	if (key_size > hash->block_size){
		printf("WARNING: in %s, key size is larger than block size\n", __func__);
		key_size = hash->block_size;
	}

	if (hash->hash_size > hash->block_size){
		printf("ERROR: in %s, hash size is larger than block size\n", __func__);
		return -1;
	}

	if ((block = malloc(hash->block_size + hash->hash_size)) == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	memset(block, 0, hash->block_size);
	memcpy(block, key, key_size);

	for (i = 0; i < hash->block_size; i++){
		block[i] ^= IPAD;
	}

	hash->func_init(hash->state);
	hash->func_feed(hash->state, block, hash->block_size);
	hash->func_feed(hash->state, input, size);
	hash->func_hash(hash->state, block + hash->block_size);

	for (i = 0; i < hash->block_size; i++){
		block[i] ^= IPAD ^ OPAD;
	}

	hash->func_init(hash->state);
	hash->func_feed(hash->state, block, hash->block_size + hash->hash_size);
	hash->func_hash(hash->state, output);

	free(block);

	return 0;
}
