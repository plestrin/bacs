#ifndef TEA_H
#define TEA_H

#include <stdint.h>

#define TEA_BLOCK_NB_BIT 	64
#define TEA_BLOCK_NB_BYTE 	8
#define TEA_KEY_NB_BIT 		128
#define TEA_KEY_NB_BYTE 	16

/* 
 * No padding is done, make sure the data length is a multiple of the block size.
 * - data			: input message. It's length is a multiple of the block size.
 * - data_length 	: size of the input
 * - key			: 128 bits key
 * - output 		: output message. Its length is equal to the length of the input
 */

void tea_encipher(uint32_t* data, uint64_t data_length, uint32_t* key, uint32_t* output);
void tea_decipher(uint32_t* data, uint64_t data_length, uint32_t* key, uint32_t* output);

void xtea_encipher(uint32_t* data, uint64_t data_length, uint32_t* key, uint32_t* output);
void xtea_decipher(uint32_t* data, uint64_t data_length, uint32_t* key, uint32_t* output);

#endif
