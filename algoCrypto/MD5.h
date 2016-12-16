#ifndef MD5_H
#define MD5_H

#include <stdint.h>

#define MD5_FAST

#define MD5_BLOCK_NB_BIT 	512
#define MD5_BLOCK_NB_BYTE 	64
#define MD5_BLOCK_NB_DWORD 	16
#define MD5_HASH_NB_BIT 	128
#define MD5_HASH_NB_BYTE 	16
#define MD5_HASH_NB_DWORD 	4

#define MD5_DATA_SIZE_TO_NB_BLOCK(size) ((((size) * 8 + 65) / MD5_BLOCK_NB_BIT) + ((((size) * 8 + 65) % MD5_BLOCK_NB_BIT == 0)?0:1))

/*
 * Make sur the data buffer is large enough to hold the padding. Use the macro above to compute the size of the data buffer
 * - data			: input message
 * - data_length 	: size of the input message
 * - hash			: 128 bits hash value allocated by the user
 */

void md5(uint32_t* data, uint64_t data_length, uint32_t* hash);

struct md5State{
	uint64_t 	global_size;
	uint32_t 	local_size;
	uint32_t 	block[MD5_BLOCK_NB_DWORD];
	uint32_t 	state[MD5_HASH_NB_DWORD];
};

void md5_init(struct md5State* md5_state);
void md5_feed(struct md5State* md5_state, uint32_t* data, uint64_t data_length);
void md5_hash(struct md5State* md5_state, uint32_t* hash);

#endif
