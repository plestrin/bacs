#ifndef RC4_H
#define RC4_H

#include <stdint.h>

#define RC4_KEY_MAX_NB_BYTE 	256
#define RC4_KEY_MIN_NB_BYTE 	1

/* 
 * Operands description:
 * - input			: input message. No constrain on the length
 * - input_length 	: size of the input
 * - key			: key. Its size range from 1 to 256 bytes
 * - key_length		: size of the key
 * - output 		: output message. Its length is equal to the length of the input
 */

void rc4(uint8_t* input, uint64_t input_length, uint8_t* key, uint8_t key_length, uint8_t* output);

#endif