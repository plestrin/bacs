#include <stdint.h>

#include "AES.h"

void wrapper_aes128_key_expand_encrypt(void** input, void** output){
	aes128_key_expand_encrypt((uint32_t*)input[0], (uint32_t*)output[0]);
}

void wrapper_aes128_key_expand_decrypt(void** input, void** output){
	aes128_key_expand_decrypt((uint32_t*)input[0], (uint32_t*)output[0]);
}

void wrapper_aes128_encrypt(void** input, void** output){
	aes128_encrypt((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes128_decrypt(void** input, void** output){
	aes128_decrypt((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes128_inner_loop_enc(void** input, void** output){
	aes128_inner_loop_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes128_inner_loop_dec(void** input, void** output){
	aes128_inner_loop_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes192_key_expand_encrypt(void** input, void** output){
	aes192_key_expand_encrypt((uint32_t*)input[0], (uint32_t*)output[0]);
}

void wrapper_aes192_inner_loop_enc(void** input, void** output){
	aes192_inner_loop_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes192_inner_loop_dec(void** input, void** output){
	aes192_inner_loop_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes256_key_expand_encrypt(void** input, void** output){
	aes256_key_expand_encrypt((uint32_t*)input[0], (uint32_t*)output[0]);
}

void wrapper_aes256_inner_loop_enc(void** input, void** output){
	aes256_inner_loop_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes256_inner_loop_dec(void** input, void** output){
	aes256_inner_loop_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}