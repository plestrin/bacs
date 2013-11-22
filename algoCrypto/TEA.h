#ifndef TEA_H
#define TEA_H

#include <stdint.h>

#define TEA_BLOCK_NB_BIT 	64
#define TEA_BLOCK_NB_BYTE 	8
#define TEA_KEY_NB_BIT 		128
#define TEA_KEY_NB_BYTE 	16

/* 
 * No padding is done, make sure the data length is a multiple of the block size. The output overwrites the input.
 * - data		: input and output message. It's length is a multiple of the block size.
 * - nb_block 	: size of the input buffer divided by the block size
 * - key		: 128 bits key
 */

void tea_encypher(uint32_t* data, uint64_t nb_block, uint32_t* key);
void tea_decipher(uint32_t* data, uint64_t nb_block, uint32_t* key);

#endif
