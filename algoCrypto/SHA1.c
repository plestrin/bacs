#include "SHA1.h"

#define F0(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define F1(x, y, z) ((x) ^ (y) ^ (z))
#define F2(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define F3(x, y, z) ((x) ^ (y) ^ (z))

#define K0 0x5a827999
#define K1 0x6ed9eba1
#define K2 0x8f1bbcdc
#define K3 0xca62c1d6

#define H0 0x67452301
#define H1 0xefcdab89
#define H2 0x98badcfe
#define H3 0x10325476
#define H4 0xc3d2e1f0

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

void sha1(uint32_t* data, uint64_t data_length, uint32_t* hash){
	uint32_t nb_block;
	uint32_t i;
	uint32_t j;
	uint32_t w[80];
	uint32_t h0 = H0;
	uint32_t h1 = H1;
	uint32_t h2 = H2;
	uint32_t h3 = H3;
	uint32_t h4 = H4;
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t tmp;

	#ifndef ANALYSIS_REFERENCE_IMPLEMENTATION

	nb_block = (uint32_t)SHA1_DATA_SIZE_TO_NB_BLOCK(data_length);

	/* Padding - no padding for reference implementation */
	*((char*)data + data_length) = (char)0x80;
	for (i = 1; (data_length + i)%SHA1_BLOCK_NB_BYTE != 56; i++){
		*((char*)data + data_length + i) = 0x00;
	}
	*((char*)data + data_length + i + 0) = data_length >> 53;
	*((char*)data + data_length + i + 1) = data_length >> 45;
	*((char*)data + data_length + i + 2) = data_length >> 37;
	*((char*)data + data_length + i + 3) = data_length >> 29;
	*((char*)data + data_length + i + 4) = data_length >> 21;
	*((char*)data + data_length + i + 5) = data_length >> 13;
	*((char*)data + data_length + i + 6) = data_length >> 5;
	*((char*)data + data_length + i + 7) = data_length << 3;

	#else

	nb_block = (uint32_t)(data_length / SHA1_BLOCK_NB_BYTE);

	#endif

	for (i = 0; i < nb_block; i++){
		for(j = 0; j < 16; j++){
			w[j]  = (data[i*16 + j] & 0x000000ff) << 24;
			w[j] |= (data[i*16 + j] & 0x0000ff00) << 8;
			w[j] |= (data[i*16 + j] & 0x00ff0000) >> 8;
			w[j] |= (data[i*16 + j] & 0xff000000) >> 24;
		}

		for(j = 16; j < 80; j++){
			w[j] = ROTATE_LEFT(w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16], 1);
		}

		a = h0;
		b = h1;
		c = h2;
		d = h3;
		e = h4;

		for (j = 0; j < 80; j++){
			if (j < 20){
				tmp = ROTATE_LEFT(a, 5) + F0(b, c, d) + e + w[j] + K0;
				e = d;
				d = c;
				c = ROTATE_LEFT(b, 30);
				b = a;
				a = tmp;
			}
			else if (j < 40){
				tmp = ROTATE_LEFT(a, 5) + F1(b, c, d) + e + w[j] + K1;
				e = d;
				d = c;
				c = ROTATE_LEFT(b, 30);
				b = a;
				a = tmp;
			}
			else if (j < 60){
				tmp = ROTATE_LEFT(a, 5) + F2(b, c, d) + e + w[j] + K2;
				e = d;
				d = c;
				c = ROTATE_LEFT(b, 30);
				b = a;
				a = tmp;
			}
			else{
				tmp = ROTATE_LEFT(a, 5) + F3(b, c, d) + e + w[j] + K3;
				e = d;
				d = c;
				c = ROTATE_LEFT(b, 30);
				b = a;
				a = tmp;
			}
		}

		h0 += a;
		h1 += b;
		h2 += c;
		h3 += d;
		h4 += e;
	}

	hash[0] = h0;
	hash[1] = h1;
	hash[2] = h2;
	hash[3] = h3;
	hash[4] = h4;
}