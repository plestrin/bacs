#include "SHA1.h"

#ifdef __GNUC__
#   define SHA1_LOAD_WORD(w)        __builtin_bswap32(w)
#   define SHA1_STORE_WORD(w)       __builtin_bswap32(w)
#else
#   define SHA1_LOAD_WORD(w)        (((w) >> 24) | ((((w) >> 16) & 0xff) << 8) | ((((w) >> 8) & 0xff) << 16) | ((w) << 24))
#   define SHA1_STORE_WORD(w)       (((w) >> 24) | ((((w) >> 16) & 0xff) << 8) | ((((w) >> 8) & 0xff) << 16) | ((w) << 24))
#endif

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
	*((uint8_t*)data + data_length) = (uint8_t)0x80;
	for (i = 1; (data_length + i)%SHA1_BLOCK_NB_BYTE != 56; i++){
		*((uint8_t*)data + data_length + i) = 0x00;
	}
	*((uint8_t*)data + data_length + i + 0) = (uint8_t)(data_length >> 53);
	*((uint8_t*)data + data_length + i + 1) = (uint8_t)(data_length >> 45);
	*((uint8_t*)data + data_length + i + 2) = (uint8_t)(data_length >> 37);
	*((uint8_t*)data + data_length + i + 3) = (uint8_t)(data_length >> 29);
	*((uint8_t*)data + data_length + i + 4) = (uint8_t)(data_length >> 21);
	*((uint8_t*)data + data_length + i + 5) = (uint8_t)(data_length >> 13);
	*((uint8_t*)data + data_length + i + 6) = (uint8_t)(data_length >> 5);
	*((uint8_t*)data + data_length + i + 7) = (uint8_t)(data_length << 3);

	#else

	nb_block = (uint32_t)(data_length / SHA1_BLOCK_NB_BYTE);

	#endif

	for (i = 0; i < nb_block; i++){
		w[0 ] = SHA1_LOAD_WORD(data[i*16 + 0 ]);
		w[1 ] = SHA1_LOAD_WORD(data[i*16 + 1 ]);
		w[2 ] = SHA1_LOAD_WORD(data[i*16 + 2 ]);
		w[3 ] = SHA1_LOAD_WORD(data[i*16 + 3 ]);
		w[4 ] = SHA1_LOAD_WORD(data[i*16 + 4 ]);
		w[5 ] = SHA1_LOAD_WORD(data[i*16 + 5 ]);
		w[6 ] = SHA1_LOAD_WORD(data[i*16 + 6 ]);
		w[7 ] = SHA1_LOAD_WORD(data[i*16 + 7 ]);
		w[8 ] = SHA1_LOAD_WORD(data[i*16 + 8 ]);
		w[9 ] = SHA1_LOAD_WORD(data[i*16 + 9 ]);
		w[10] = SHA1_LOAD_WORD(data[i*16 + 10]);
		w[11] = SHA1_LOAD_WORD(data[i*16 + 11]);
		w[12] = SHA1_LOAD_WORD(data[i*16 + 12]);
		w[13] = SHA1_LOAD_WORD(data[i*16 + 13]);
		w[14] = SHA1_LOAD_WORD(data[i*16 + 14]);
		w[15] = SHA1_LOAD_WORD(data[i*16 + 15]);

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

	hash[0] = SHA1_STORE_WORD(h0);
	hash[1] = SHA1_STORE_WORD(h1);
	hash[2] = SHA1_STORE_WORD(h2);
	hash[3] = SHA1_STORE_WORD(h3);
	hash[4] = SHA1_STORE_WORD(h4);
}