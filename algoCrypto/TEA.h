#ifndef TEA_H
#define TEA_H

#include <stdint.h>

#define TEA_BLOCK_NB_BIT 	64
#define TEA_BLOCK_NB_BYTE 	8
#define TEA_BLOCK_NB_DWORD 	2
#define TEA_KEY_NB_BIT 		128
#define TEA_KEY_NB_BYTE 	16
#define TEA_KEY_NB_DWORD 	4

/* 
 * No padding is done, make sure the data length is a multiple of the block size.
 * - data			: one block of input message
 * - key			: 128 bits key
 * - output 		: one block of output message
 */

void tea_encrypt(const uint32_t* data, const uint32_t* key, uint32_t* output);
void tea_decrypt(const uint32_t* data, const uint32_t* key, uint32_t* output);

void xtea_encrypt(const uint32_t* data, const uint32_t* key, uint32_t* output);
void xtea_decrypt(const uint32_t* data, const uint32_t* key, uint32_t* output);

#endif
