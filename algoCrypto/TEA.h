#ifndef TEA_H
#define TEA_H

#include <stdint.h>

#define TEA_BLOCK_NB_BIT 	64
#define TEA_BLOCK_NB_BYTE 	8

void tea_encypher(uint32_t* data, uint64_t nb_block, uint32_t* key);
void tea_decipher(uint32_t* data, uint64_t nb_block, uint32_t* key);

#endif
