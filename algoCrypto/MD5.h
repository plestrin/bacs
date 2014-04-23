#ifndef MD5_H
#define MD5_H

#include <stdint.h>

#define MD5_BLOCK_NB_BIT 	512
#define MD5_BLOCK_NB_BYTE 	64
#define MD5_BLOCK_NB_WORD 	16
#define MD5_HASH_NB_BIT 	128
#define MD5_HASH_NB_BYTE 	16
#define MD5_HASH_NB_WORD 	4

#define MD5_DATA_SIZE_TO_NB_BLOCK(size) ((((size) * 8 + 65) / MD5_BLOCK_NB_BIT) + ((((size) * 8 + 65) % MD5_BLOCK_NB_BIT == 0)?0:1))

/* 
 * Make sur the data buffer is large enough to hold the padding. Use the macro above to compute the size of the data buffer
 * - data			: input message
 * - data_length 	: size of the input message
 * - hash			: 128 bits hash value allocated by the user
 */

void md5(uint32_t* data, uint64_t data_length, uint32_t* hash);

#ifdef ANALYSIS_REFERENCE_IMPLEMENTATION

/* 
 * - state_input	: (A | B | C | D) at the beginning of the given round (128 bits)
 * - data 			: block of data (512 bits)
 * - state_output	: (A | B | C | D) at the end of the given round (128 bits)
 */

void md5_round1(uint32_t* state_input, uint32_t* data, uint32_t* state_output);
void md5_round2(uint32_t* state_input, uint32_t* data, uint32_t* state_output);
void md5_round3(uint32_t* state_input, uint32_t* data, uint32_t* state_output);
void md5_round4(uint32_t* state_input, uint32_t* data, uint32_t* state_output);

#endif

#endif