#ifndef SERPENT_H
#define SERPENT_H

#include <stdint.h>

#define SERPENT_BLOCK_NB_BIT 		128
#define SERPENT_BLOCK_NB_BYTE 		16
#define SERPENT_BLOCK_NB_DWORD 		4

#define SERPENT_KEY_MAX_NB_BIT 		256
#define SERPENT_KEY_MAX_NB_BYTE 	32
#define SERPENT_KEY_MAX_NB_DWORD 	8

#define SERPENT_ROUND_KEY_NB_BIT 	4224
#define SERPENT_ROUND_KEY_NB_BYTE 	528
#define SERPENT_ROUND_KEY_NB_DWORD 	132

#define SERPENT_NB_ROUND 			32

/* 
 * Key schedule
 * - key			: any size up to 256 bits. The buffer must be at least 256 bits large
 * - key_length 	: the key length in bit
 * - round_key 		: 132 words
 */

void serpent_key_expand(uint32_t* key, uint32_t key_length, uint32_t* round_key);

/* 
 * Encrypt / decrypt one block of data (128 bits)
 * - input			: 128 bits (plaintext for encryption and ciphertext for decryption)
 * - round_key 		: 132 words
 * - output 		: 128 bits (ciphertext for encryption and plaintext for decryption)
 */

void serpent_encrypt(const uint32_t* input, const uint32_t* round_key, uint32_t* output);
void serpent_decrypt(const uint32_t* input, const uint32_t* round_key, uint32_t* output);

#endif