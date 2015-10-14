#ifndef TWOFISH_H
#define TWOFISH_H

#include <stdint.h>

#define TWOFISH_128_NB_BIT_KEY 		128
#define TWOFISH_128_NB_BYTE_KEY		16
#define TWOFISH_128_NB_DWORD_KEY 	4

#define TWOFISH_192_NB_BIT_KEY 		192
#define TWOFISH_192_NB_BYTE_KEY 	24
#define TWOFISH_192_NB_DWORD_KEY 	6

#define TWOFISH_256_NB_BIT_KEY 		256
#define TWOFISH_256_NB_BYTE_KEY 	32
#define TWOFISH_256_NB_DWORD_KEY 	8

#define TWOFISH_BLOCK_NB_BIT 		128
#define TWOFISH_BLOCK_NB_BYTE 		16

#define TWOFISH_NB_ROUND 			8

#define TWOFISH_NB_ROUND_KEY 		(8 + 4 *TWOFISH_NB_ROUND)
#define TWOFISH_NB_BIT_ROUND_KEY	(32 * TWOFISH_NB_ROUND_KEY)
#define TWOFISH_NB_BYTE_ROUND_KEY	(4 * TWOFISH_NB_ROUND_KEY)
#define TWOFISH_NB_DWORD_ROUND_KEY	TWOFISH_NB_ROUND_KEY

struct twofishKey{
	uint32_t 	ks[TWOFISH_NB_DWORD_ROUND_KEY];
	uint32_t 	sbox[4][256];
};

/* 
 * Key schedule
 * - key			: 128, 192 or 256 bits
 * - twofish_key 	: pointer to the structure containing the round keys and the SBOX derived from the key
 */

void twofish128_key_init(const uint32_t* key, struct twofishKey* twofish_key);
void twofish192_key_init(const uint32_t* key, struct twofishKey* twofish_key);
void twofish256_key_init(const uint32_t* key, struct twofishKey* twofish_key);

/* 
 * Encrypt / decrypt one block of data (128 bits)
 * - input			: 128 bits (plaintext for encryption and ciphertext for decryption)
 * - twofish_key 	: pointer to the structure containing the round keys and the SBOX derived from the key
 * - output 		: 128 bits (ciphertext for encryption and plaintext for decryption)
 */

void twofish_encrypt(const uint32_t* input, const struct twofishKey* twofish_key, uint32_t* output);
void twofish_decrypt(const uint32_t* input, const struct twofishKey* twofish_key, uint32_t* output);

#endif