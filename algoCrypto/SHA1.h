#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>

#define SHA1_BLOCK_NB_BIT 	512
#define SHA1_BLOCK_NB_BYTE 	64
#define SHA1_BLOCK_NB_WORD 	16
#define SHA1_HASH_NB_BIT 	160
#define SHA1_HASH_NB_BYTE 	20
#define SHA1_HASH_NB_WORD 	5

#define SHA1_DATA_SIZE_TO_NB_BLOCK(size) ((((size) * 8 + 65) / SHA1_BLOCK_NB_BIT) + ((((size) * 8 + 65) % SHA1_BLOCK_NB_BIT == 0)?0:1))

/* 
 * Make sur the data buffer is large enough to hold the padding. Use the macro above to compute the size of the data buffer
 * - data			: input message
 * - data_length 	: size of the input message
 * - hash			: 160 bits hash value allocated by the user
 */

void sha1(uint32_t* data, uint64_t data_length, uint32_t* hash);

#endif