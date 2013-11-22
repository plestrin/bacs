#ifndef MD5_H
#define MD5_H

#include <stdint.h>

#define MD5_BLOCK_NB_BIT 	512
#define MD5_BLOCK_NB_BYTE 	64
#define MD5_HASH_NB_BIT 	128
#define MD5_HASH_NB_BYTE 	16

#define MD5_DATA_SIZE_TO_NB_BLOCK(size) (((size) * 8 + 65) / MD5_BLOCK_NB_BIT + (((size) * 8 + 65) % MD5_BLOCK_NB_BIT == 0)?0:1)

/* 
 * Make sur the data buffer is large enough to hold the padding. Use the macro above to compute the size of the data buffer
 * - data		: input message
 * - nb_block 	: size of the input message
 * - hash		: 128 bits hash value allocated by the user
 */

void md5(uint32_t* data, uint64_t data_length, uint32_t* hash);

#endif