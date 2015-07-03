#include "MD5.h"

#ifdef MD5_FAST
	#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
	#define G(x, y, z) ((y) ^ ((z) & ((y) ^ (x))))
#else
	#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
	#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#endif
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define FF(a, b, c, d, x, s, i) ((a) = (b) + ROTATE_LEFT(((a) + F((b), (c), (d)) + (x) + (i)), (s)))
#define GG(a, b, c, d, x, s, i) ((a) = (b) + ROTATE_LEFT(((a) + G((b), (c), (d)) + (x) + (i)), (s)))
#define HH(a, b, c, d, x, s, i) ((a) = (b) + ROTATE_LEFT(((a) + H((b), (c), (d)) + (x) + (i)), (s)))
#define II(a, b, c, d, x, s, i) ((a) = (b) + ROTATE_LEFT(((a) + I((b), (c), (d)) + (x) + (i)), (s)))

void md5(uint32_t* data, uint64_t data_length, uint32_t* hash){
	uint32_t AA = 0x67452301;
	uint32_t BB = 0xefcdab89;
	uint32_t CC = 0x98badcfe;
	uint32_t DD = 0x10325476;

	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;

	uint32_t nb_block;
	uint32_t i;

	#ifndef ANALYSIS_REFERENCE_IMPLEMENTATION

	nb_block = (uint32_t)MD5_DATA_SIZE_TO_NB_BLOCK(data_length);

	/* Padding - no padding for reference implementation */
	*((char*)data + data_length) = (char)0x80;
	for (i = 1; (data_length + i)%MD5_BLOCK_NB_BYTE != 56; i++){
		*((char*)data + data_length + i) = 0x00;
	}
	*(uint64_t*)((char*)data + data_length + i) = data_length * 8;

	#else

	nb_block = (uint32_t)(data_length / MD5_BLOCK_NB_BYTE);

	#endif

	for (i = 0; i < nb_block; i++){

		A = AA;
		B = BB;
		C = CC;
		D = DD;

		/* Round 1 */
		FF(A, B, C, D, data[i*16 +  0],  7, 0xd76aa478);
		FF(D, A, B, C, data[i*16 +  1], 12, 0xe8c7b756);
		FF(C, D, A, B, data[i*16 +  2], 17, 0x242070db);
		FF(B, C, D, A, data[i*16 +  3], 22, 0xc1bdceee);
		FF(A, B, C, D, data[i*16 +  4],  7, 0xf57c0faf);
		FF(D, A, B, C, data[i*16 +  5], 12, 0x4787c62a);
		FF(C, D, A, B, data[i*16 +  6], 17, 0xa8304613);
		FF(B, C, D, A, data[i*16 +  7], 22, 0xfd469501);
		FF(A, B, C, D, data[i*16 +  8],  7, 0x698098d8);
		FF(D, A, B, C, data[i*16 +  9], 12, 0x8b44f7af);
		FF(C, D, A, B, data[i*16 + 10], 17, 0xffff5bb1);
		FF(B, C, D, A, data[i*16 + 11], 22, 0x895cd7be);
		FF(A, B, C, D, data[i*16 + 12],  7, 0x6b901122);
		FF(D, A, B, C, data[i*16 + 13], 12, 0xfd987193);
		FF(C, D, A, B, data[i*16 + 14], 17, 0xa679438e);
		FF(B, C, D, A, data[i*16 + 15], 22, 0x49b40821);

		/* Round 2 */
		GG(A, B, C, D, data[i*16 +  1],  5, 0xf61e2562);
		GG(D, A, B, C, data[i*16 +  6],  9, 0xc040b340);
		GG(C, D, A, B, data[i*16 + 11], 14, 0x265e5a51);
		GG(B, C, D, A, data[i*16 +  0], 20, 0xe9b6c7aa);
		GG(A, B, C, D, data[i*16 +  5],  5, 0xd62f105d);
		GG(D, A, B, C, data[i*16 + 10],  9, 0x02441453);
		GG(C, D, A, B, data[i*16 + 15], 14, 0xd8a1e681);
		GG(B, C, D, A, data[i*16 +  4], 20, 0xe7d3fbc8);
		GG(A, B, C, D, data[i*16 +  9],  5, 0x21e1cde6);
		GG(D, A, B, C, data[i*16 + 14],  9, 0xc33707d6);
		GG(C, D, A, B, data[i*16 +  3], 14, 0xf4d50d87);
		GG(B, C, D, A, data[i*16 +  8], 20, 0x455a14ed);
		GG(A, B, C, D, data[i*16 + 13],  5, 0xa9e3e905);
		GG(D, A, B, C, data[i*16 +  2],  9, 0xfcefa3f8);
		GG(C, D, A, B, data[i*16 +  7], 14, 0x676f02d9);
		GG(B, C, D, A, data[i*16 + 12], 20, 0x8d2a4c8a);

		/* Round 3 */
		HH(A, B, C, D, data[i*16 +  5],  4, 0xfffa3942);
		HH(D, A, B, C, data[i*16 +  8], 11, 0x8771f681);
		HH(C, D, A, B, data[i*16 + 11], 16, 0x6d9d6122);
		HH(B, C, D, A, data[i*16 + 14], 23, 0xfde5380c);
		HH(A, B, C, D, data[i*16 +  1],  4, 0xa4beea44);
		HH(D, A, B, C, data[i*16 +  4], 11, 0x4bdecfa9);
		HH(C, D, A, B, data[i*16 +  7], 16, 0xf6bb4b60);
		HH(B, C, D, A, data[i*16 + 10], 23, 0xbebfbc70);
		HH(A, B, C, D, data[i*16 + 13],  4, 0x289b7ec6);
		HH(D, A, B, C, data[i*16 +  0], 11, 0xeaa127fa);
		HH(C, D, A, B, data[i*16 +  3], 16, 0xd4ef3085);
		HH(B, C, D, A, data[i*16 +  6], 23, 0x04881d05);
		HH(A, B, C, D, data[i*16 +  9],  4, 0xd9d4d039);
		HH(D, A, B, C, data[i*16 + 12], 11, 0xe6db99e5);
		HH(C, D, A, B, data[i*16 + 15], 16, 0x1fa27cf8);
		HH(B, C, D, A, data[i*16 +  2], 23, 0xc4ac5665);

		/* Round 4 */
		II(A, B, C, D, data[i*16 +  0],  6, 0xf4292244);
		II(D, A, B, C, data[i*16 +  7], 10, 0x432aff97);
		II(C, D, A, B, data[i*16 + 14], 15, 0xab9423a7);
		II(B, C, D, A, data[i*16 +  5], 21, 0xfc93a039);
		II(A, B, C, D, data[i*16 + 12],  6, 0x655b59c3);
		II(D, A, B, C, data[i*16 +  3], 10, 0x8f0ccc92);
		II(C, D, A, B, data[i*16 + 10], 15, 0xffeff47d);
		II(B, C, D, A, data[i*16 +  1], 21, 0x85845dd1);
		II(A, B, C, D, data[i*16 +  8],  6, 0x6fa87e4f);
		II(D, A, B, C, data[i*16 + 15], 10, 0xfe2ce6e0);
		II(C, D, A, B, data[i*16 +  6], 15, 0xa3014314);
		II(B, C, D, A, data[i*16 + 13], 21, 0x4e0811a1);
		II(A, B, C, D, data[i*16 +  4],  6, 0xf7537e82);
		II(D, A, B, C, data[i*16 + 11], 10, 0xbd3af235);
		II(C, D, A, B, data[i*16 +  2], 15, 0x2ad7d2bb);
		II(B, C, D, A, data[i*16 +  9], 21, 0xeb86d391);

		AA += A;
		BB += B;
		CC += C;
		DD += D;
	}

	hash[0] = AA;
	hash[1] = BB;
	hash[2] = CC;
	hash[3] = DD;
}

#ifdef ANALYSIS_REFERENCE_IMPLEMENTATION

void md5_round1(uint32_t* state_input, uint32_t* data, uint32_t* state_output){
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;

	A = state_input[0];
	B = state_input[1];
	C = state_input[2];
	D = state_input[3];

	FF(A, B, C, D, data[ 0],  7, 0xd76aa478);
	FF(D, A, B, C, data[ 1], 12, 0xe8c7b756);
	FF(C, D, A, B, data[ 2], 17, 0x242070db);
	FF(B, C, D, A, data[ 3], 22, 0xc1bdceee);
	FF(A, B, C, D, data[ 4],  7, 0xf57c0faf);
	FF(D, A, B, C, data[ 5], 12, 0x4787c62a);
	FF(C, D, A, B, data[ 6], 17, 0xa8304613);
	FF(B, C, D, A, data[ 7], 22, 0xfd469501);
	FF(A, B, C, D, data[ 8],  7, 0x698098d8);
	FF(D, A, B, C, data[ 9], 12, 0x8b44f7af);
	FF(C, D, A, B, data[10], 17, 0xffff5bb1);
	FF(B, C, D, A, data[11], 22, 0x895cd7be);
	FF(A, B, C, D, data[12],  7, 0x6b901122);
	FF(D, A, B, C, data[13], 12, 0xfd987193);
	FF(C, D, A, B, data[14], 17, 0xa679438e);
	FF(B, C, D, A, data[15], 22, 0x49b40821);

	state_output[0] = A;
	state_output[1] = B;
	state_output[2] = C;
	state_output[3] = D;
}

void md5_round2(uint32_t* state_input, uint32_t* data, uint32_t* state_output){
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;

	A = state_input[0];
	B = state_input[1];
	C = state_input[2];
	D = state_input[3];

	GG(A, B, C, D, data[ 1],  5, 0xf61e2562);
	GG(D, A, B, C, data[ 6],  9, 0xc040b340);
	GG(C, D, A, B, data[11], 14, 0x265e5a51);
	GG(B, C, D, A, data[ 0], 20, 0xe9b6c7aa);
	GG(A, B, C, D, data[ 5],  5, 0xd62f105d);
	GG(D, A, B, C, data[10],  9, 0x02441453);
	GG(C, D, A, B, data[15], 14, 0xd8a1e681);
	GG(B, C, D, A, data[ 4], 20, 0xe7d3fbc8);
	GG(A, B, C, D, data[ 9],  5, 0x21e1cde6);
	GG(D, A, B, C, data[14],  9, 0xc33707d6);
	GG(C, D, A, B, data[ 3], 14, 0xf4d50d87);
	GG(B, C, D, A, data[ 8], 20, 0x455a14ed);
	GG(A, B, C, D, data[13],  5, 0xa9e3e905);
	GG(D, A, B, C, data[ 2],  9, 0xfcefa3f8);
	GG(C, D, A, B, data[ 7], 14, 0x676f02d9);
	GG(B, C, D, A, data[12], 20, 0x8d2a4c8a);

	state_output[0] = A;
	state_output[1] = B;
	state_output[2] = C;
	state_output[3] = D;
}

void md5_round3(uint32_t* state_input, uint32_t* data, uint32_t* state_output){
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;

	A = state_input[0];
	B = state_input[1];
	C = state_input[2];
	D = state_input[3];

	HH(A, B, C, D, data[ 5],  4, 0xfffa3942);
	HH(D, A, B, C, data[ 8], 11, 0x8771f681);
	HH(C, D, A, B, data[11], 16, 0x6d9d6122);
	HH(B, C, D, A, data[14], 23, 0xfde5380c);
	HH(A, B, C, D, data[ 1],  4, 0xa4beea44);
	HH(D, A, B, C, data[ 4], 11, 0x4bdecfa9);
	HH(C, D, A, B, data[ 7], 16, 0xf6bb4b60);
	HH(B, C, D, A, data[10], 23, 0xbebfbc70);
	HH(A, B, C, D, data[13],  4, 0x289b7ec6);
	HH(D, A, B, C, data[ 0], 11, 0xeaa127fa);
	HH(C, D, A, B, data[ 3], 16, 0xd4ef3085);
	HH(B, C, D, A, data[ 6], 23, 0x04881d05);
	HH(A, B, C, D, data[ 9],  4, 0xd9d4d039);
	HH(D, A, B, C, data[12], 11, 0xe6db99e5);
	HH(C, D, A, B, data[15], 16, 0x1fa27cf8);
	HH(B, C, D, A, data[ 2], 23, 0xc4ac5665);

	state_output[0] = A;
	state_output[1] = B;
	state_output[2] = C;
	state_output[3] = D;
}

void md5_round4(uint32_t* state_input, uint32_t* data, uint32_t* state_output){
		uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;

	A = state_input[0];
	B = state_input[1];
	C = state_input[2];
	D = state_input[3];

	II(A, B, C, D, data[ 0],  6, 0xf4292244);
	II(D, A, B, C, data[ 7], 10, 0x432aff97);
	II(C, D, A, B, data[14], 15, 0xab9423a7);
	II(B, C, D, A, data[ 5], 21, 0xfc93a039);
	II(A, B, C, D, data[12],  6, 0x655b59c3);
	II(D, A, B, C, data[ 3], 10, 0x8f0ccc92);
	II(C, D, A, B, data[10], 15, 0xffeff47d);
	II(B, C, D, A, data[ 1], 21, 0x85845dd1);
	II(A, B, C, D, data[ 8],  6, 0x6fa87e4f);
	II(D, A, B, C, data[15], 10, 0xfe2ce6e0);
	II(C, D, A, B, data[ 6], 15, 0xa3014314);
	II(B, C, D, A, data[13], 21, 0x4e0811a1);
	II(A, B, C, D, data[ 4],  6, 0xf7537e82);
	II(D, A, B, C, data[11], 10, 0xbd3af235);
	II(C, D, A, B, data[ 2], 15, 0x2ad7d2bb);
	II(B, C, D, A, data[ 9], 21, 0xeb86d391);
	
	state_output[0] = A;
	state_output[1] = B;
	state_output[2] = C;
	state_output[3] = D;
}

#endif