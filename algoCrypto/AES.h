#ifndef AES_H
#define AES_H

#include <stdint.h>

#define AES_128_NB_BIT_KEY 			128
#define AES_128_NB_BYTE_KEY			16
#define AES_128_NB_WORD_KEY 		4
#define AES_128_NB_BIT_ROUND_KEY	1408
#define AES_128_NB_BYTE_ROUND_KEY	176
#define AES_128_NB_WORD_ROUND_KEY	44
#define AES_128_NB_ROUND 			10

#define AES_192_NB_BIT_KEY 			192
#define AES_192_NB_BYTE_KEY 		24
#define AES_192_NB_WORD_KEY 		6
#define AES_192_NB_BIT_ROUND_KEY	1664
#define AES_192_NB_BYTE_ROUND_KEY	208
#define AES_192_NB_WORD_ROUND_KEY	52
#define AES_192_NB_ROUND 			12

#define AES_256_NB_BIT_KEY 			256
#define AES_256_NB_BYTE_KEY 		32
#define AES_256_NB_WORD_KEY 		8
#define AES_256_NB_BIT_ROUND_KEY 	1920
#define AES_256_NB_BYTE_ROUND_KEY 	240
#define AES_256_NB_WORD_ROUND_KEY 	60
#define AES_256_NB_ROUND 			14

#define AES_BLOCK_NB_BIT 			128
#define AES_BLOCK_NB_BYTE 			16

/* 
 * Key schedule - normally the same round key is used for encryption and decryption but here it's a little bit tricky due to our implementation
 * - key			: 128, 192 or 256 bits
 * - round_key 		: 44, 52, 60 words
 */

void aes128_key_expand_encrypt(uint32_t* key, uint32_t* round_key);
void aes128_key_expand_decrypt(uint32_t* key, uint32_t* round_key);

void aes192_key_expand_encrypt(uint32_t* key, uint32_t* round_key);
void aes192_key_expand_decrypt(uint32_t* key, uint32_t* round_key);

void aes256_key_expand_encrypt(uint32_t* key, uint32_t* round_key);
void aes256_key_expand_decrypt(uint32_t* key, uint32_t* round_key);

/* 
 * Encrypt / decrypt one block of data (128 bits)
 * - input			: 128 bits (plaintext for encryption and ciphertext for decryption)
 * - round_key 		: 44, 52, 60 words
 * - output 		: 128 bits (ciphertext for encryption and plaintext for decryption)
 */

void aes128_encrypt(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes128_decrypt(uint32_t* input, uint32_t* round_key, uint32_t* output);

void aes192_encrypt(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes192_decrypt(uint32_t* input, uint32_t* round_key, uint32_t* output);

void aes256_encrypt(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes256_decrypt(uint32_t* input, uint32_t* round_key, uint32_t* output);

#ifdef ANALYSIS_REFERENCE_IMPLEMENTATION
/* 
 * Designed for the analysis program (loop centric approach)
 * - input			: 128 bits
 * - round_key 		: 36, 44, 52 words
 * - output 		: 128 bits
 */

 void aes128_inner_loop_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
 void aes128_inner_loop_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);

 void aes192_inner_loop_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
 void aes192_inner_loop_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);

 void aes256_inner_loop_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
 void aes256_inner_loop_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);

void aes_1_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_2_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_3_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_4_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_5_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_6_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_7_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_8_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_9_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_10_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_11_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_12_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_13_round_enc(uint32_t* input, uint32_t* round_key, uint32_t* output);

void aes_1_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_2_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_3_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_4_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_5_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_6_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_7_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_8_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_9_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_10_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_11_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_12_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_13_round_dec(uint32_t* input, uint32_t* round_key, uint32_t* output);

void aes_1_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_2_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_3_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_4_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_5_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_6_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_7_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_8_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_9_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_10_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_11_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_12_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);
void aes_13_round_dec_rev(uint32_t* input, uint32_t* round_key, uint32_t* output);

#endif

#endif