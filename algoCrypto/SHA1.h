#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>

#define SHA1_BLOCK_NB_BIT 	512
#define SHA1_BLOCK_NB_BYTE 	64
#define SHA1_BLOCK_NB_DWORD 16
#define SHA1_HASH_NB_BIT 	160
#define SHA1_HASH_NB_BYTE 	20
#define SHA1_HASH_NB_DWORD 	5

/*
 * - data			: input message
 * - data_length 	: size of the input message
 * - hash			: 160 bits hash value allocated by the user
 */

struct sha1State{
	uint64_t 	global_size;
	uint32_t 	local_size;
	uint32_t 	block[SHA1_BLOCK_NB_DWORD];
	uint32_t 	state[SHA1_HASH_NB_DWORD];
};

void sha1_init(struct sha1State* sha1_state);
void sha1_feed(struct sha1State* sha1_state, const uint32_t* data, size_t data_length);
void sha1_hash(struct sha1State* sha1_state, uint32_t* hash);

#endif
