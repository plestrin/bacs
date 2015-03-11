#ifndef MODE_H
#define MODE_H

#include <stdint.h>
#include <string.h>

#define MAX_BLOCK_SIZE 16

/*
 * Calling convention for block ciphers:
 * 	- arg1: input buffer
 * 	- arg2: key, round key buffer or specific key structure
 * 	- arg3: output_buffer
 */

typedef void(*blockCipher)(void*,void*,void*);

void mode_enc_ecb(blockCipher encrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key);
void mode_dec_ecb(blockCipher decrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key);

void mode_enc_cbc(blockCipher encrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key, uint8_t* iv);
void mode_dec_cbc(blockCipher decrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key, uint8_t* iv);

void mode_enc_ofb(blockCipher encrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key, uint8_t* iv);
void mode_dec_ofb(blockCipher encrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key, uint8_t* iv);

void mode_enc_cfb(blockCipher encrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key, uint8_t* iv);
void mode_dec_cfb(blockCipher encrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key, uint8_t* iv);

void mode_enc_ctr(blockCipher encrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key, uint8_t* iv);
void mode_dec_ctr(blockCipher encrypt, uint32_t block_size, uint8_t* input, uint8_t* output, size_t size, void* key, uint8_t* iv);

#endif