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

void wrapper_aes_1_round_enc(void** input, void** output){
	aes_1_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_2_round_enc(void** input, void** output){
	aes_2_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_3_round_enc(void** input, void** output){
	aes_3_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_4_round_enc(void** input, void** output){
	aes_4_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_5_round_enc(void** input, void** output){
	aes_5_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_6_round_enc(void** input, void** output){
	aes_6_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_7_round_enc(void** input, void** output){
	aes_7_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_8_round_enc(void** input, void** output){
	aes_8_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_9_round_enc(void** input, void** output){
	aes_9_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_10_round_enc(void** input, void** output){
	aes_10_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_11_round_enc(void** input, void** output){
	aes_11_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_12_round_enc(void** input, void** output){
	aes_12_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_13_round_enc(void** input, void** output){
	aes_13_round_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_1_round_dec(void** input, void** output){
	aes_1_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_2_round_dec(void** input, void** output){
	aes_2_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_3_round_dec(void** input, void** output){
	aes_3_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_4_round_dec(void** input, void** output){
	aes_4_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_5_round_dec(void** input, void** output){
	aes_5_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_6_round_dec(void** input, void** output){
	aes_6_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_7_round_dec(void** input, void** output){
	aes_7_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_8_round_dec(void** input, void** output){
	aes_8_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_9_round_dec(void** input, void** output){
	aes_9_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_10_round_dec(void** input, void** output){
	aes_10_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_11_round_dec(void** input, void** output){
	aes_11_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_12_round_dec(void** input, void** output){
	aes_12_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void wrapper_aes_13_round_dec(void** input, void** output){
	aes_13_round_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}