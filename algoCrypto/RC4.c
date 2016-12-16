#include "RC4.h"

void rc4(const uint8_t* input, size_t input_length, const uint8_t* key, size_t key_length, uint8_t* output){
	uint8_t 	S[256];
	uint8_t 	i;
	uint8_t 	j = 0;
	size_t 		n;
	uint8_t 	tmp;
	uint8_t 	key_stream;

	for (n = 0; n < 256; n++){
		S[n] = (uint8_t)n;
	}

	/* Key-scheduling algorithm (KSA) */
	for (n = 0; n < 256; n++){
		i = (uint8_t)n;
		j = j + S[i] + key[i % key_length];
		tmp = S[i];
		S[i] = S[j];
		S[j] = tmp;
	}

	/* Pseudo-random generation algorithm (PRGA) */
	i = 0;
	j = 0;
	for (n = 0; n < input_length; n++){
		i = i + 1;
		j = j + S[i];
		tmp = S[i];
		S[i] = S[j];
		S[j] = tmp;
		key_stream = S[(uint8_t)(S[i] + S[j])];
		output[n] = input[n] ^ key_stream;
	}
}
