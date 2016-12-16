#ifndef DES_H
#define DES_H

#include <stdint.h>

#define DES_BLOCK_NB_BIT 		64
#define DES_BLOCK_NB_BYTE 		8
#define DES_BLOCK_NB_DWORD 		4

#define DES_KEY_NB_BIT 			64
#define DES_KEY_NB_BYTE 		8
#define DES_KEY_NB_DWORD 		4

#define DES_ROUND_KEY_NB_BIT 	1024
#define DES_ROUND_KEY_NB_BYTE 	128
#define DES_ROUND_KEY_NB_WORD 	32

/*
 * Key schedule
 * - key			: 64 bits key
 * - round_key 		: 128 bytes round key buffer
 */

void des_key_expand(const uint8_t* key, uint8_t* round_key);

/*
 * Encrypt / decrypt one block of data (64 bits)
 * - input			: 64 bits (plaintext for encryption and ciphertext for decryption)
 * - round_key 		: 32 words
 * - output 		: 64 bits (ciphertext for encryption and plaintext for decryption)
 */

void des_encrypt(const uint32_t* input, const uint32_t* round_key, uint32_t* output);
void des_decrypt(const uint32_t* input, const uint32_t* round_key, uint32_t* output);


#endif
