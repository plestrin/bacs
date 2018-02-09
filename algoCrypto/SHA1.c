#include <string.h>

#include "SHA1.h"

#ifdef __GNUC__
# 	define SHA1_LOAD_DWORD(w) 	__builtin_bswap32(w)
# 	define SHA1_STORE_DWORD(w) 	__builtin_bswap32(w)
#else
# 	define SHA1_LOAD_DWORD(w) 	(((w) >> 24) | ((((w) >> 16) & 0xff) << 8) | ((((w) >> 8) & 0xff) << 16) | ((w) << 24))
# 	define SHA1_STORE_DWORD(w) 	(((w) >> 24) | ((((w) >> 16) & 0xff) << 8) | ((((w) >> 8) & 0xff) << 16) | ((w) << 24))
#endif

#define F0(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define F1(x, y, z) ((x) ^ (y) ^ (z))
#define F2(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define F3(x, y, z) ((x) ^ (y) ^ (z))

#define H0 0x67452301
#define H1 0xefcdab89
#define H2 0x98badcfe
#define H3 0x10325476
#define H4 0xc3d2e1f0

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

void sha1_init(struct sha1State* sha1_state){
	sha1_state->global_size = 0;
	sha1_state->local_size 	= 0;
	sha1_state->state[0] 	= H0;
	sha1_state->state[1] 	= H1;
	sha1_state->state[2] 	= H2;
	sha1_state->state[3] 	= H3;
	sha1_state->state[4] 	= H4;
}

static void sha1_compress(struct sha1State* sha1_state){
	uint32_t 		w[80];
	uint32_t 		a;
	uint32_t 		b;
	uint32_t 		c;
	uint32_t 		d;
	uint32_t 		e;
	int 			t;
	uint32_t 		tmp;

	const uint32_t 	K[] = {0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6};

	for (t = 0; t < 16; t++){
		w[t] = SHA1_LOAD_DWORD(sha1_state->block[t]);
	}

	for (t = 16; t < 80; t++){
		w[t] = ROTATE_LEFT(w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16], 1);
	}

	a = sha1_state->state[0];
	b = sha1_state->state[1];
	c = sha1_state->state[2];
	d = sha1_state->state[3];
	e = sha1_state->state[4];

	for (t = 0; t < 20; t++){
		tmp = ROTATE_LEFT(a, 5) + F0(b, c, d) + e + w[t] + K[0];
		e = d;
		d = c;
		c = ROTATE_LEFT(b, 30);
		b = a;
		a = tmp;
	}

	for (t = 20; t < 40; t++){
		tmp = ROTATE_LEFT(a, 5) + F1(b, c, d) + e + w[t] + K[1];
		e = d;
		d = c;
		c = ROTATE_LEFT(b, 30);
		b = a;
		a = tmp;
	}

	for (t = 40; t < 60; t++){
		tmp = ROTATE_LEFT(a, 5) + F2(b, c, d) + e + w[t] + K[2];
		e = d;
		d = c;
		c = ROTATE_LEFT(b, 30);
		b = a;
		a = tmp;
	}

	for (t = 60; t < 80; t++){
		tmp = ROTATE_LEFT(a, 5) + F3(b, c, d) + e + w[t] + K[3];
		e = d;
		d = c;
		c = ROTATE_LEFT(b, 30);
		b = a;
		a = tmp;
	}

	sha1_state->state[0] += a;
	sha1_state->state[1] += b;
	sha1_state->state[2] += c;
	sha1_state->state[3] += d;
	sha1_state->state[4] += e;
}

void sha1_feed(struct sha1State* sha1_state, const uint32_t* data, size_t data_length){
	sha1_state->global_size += data_length;

	while (sha1_state->local_size + data_length >= SHA1_BLOCK_NB_BYTE){
		memcpy((uint8_t*)sha1_state->block + sha1_state->local_size, data, SHA1_BLOCK_NB_BYTE - sha1_state->local_size);
		sha1_compress(sha1_state);

		data = (const uint32_t*)((const uint8_t*)data + (SHA1_BLOCK_NB_BYTE - sha1_state->local_size));
		data_length = data_length - (SHA1_BLOCK_NB_BYTE - sha1_state->local_size);
		sha1_state->local_size = 0;
	}

	if (data_length > 0){
		memcpy((uint8_t*)sha1_state->block + sha1_state->local_size, data, (uint32_t)data_length);
		sha1_state->local_size = (uint32_t)data_length;
	}
}

void sha1_hash(struct sha1State* sha1_state, uint32_t* hash){
	*((uint8_t*)sha1_state->block + (sha1_state->local_size ++)) = 0x80;

	if (sha1_state->local_size + 8 > SHA1_BLOCK_NB_BYTE){
		memset(sha1_state->block + sha1_state->local_size, 0, SHA1_BLOCK_NB_BYTE - sha1_state->local_size);
		sha1_compress(sha1_state);
		sha1_state->local_size = 0;
	}

	memset((uint8_t*)sha1_state->block + sha1_state->local_size, 0, SHA1_BLOCK_NB_BYTE - 8 - sha1_state->local_size);

	*((uint8_t*)sha1_state->block + SHA1_BLOCK_NB_BYTE - 8 + 0) = (uint8_t)(sha1_state->global_size >> 53);
	*((uint8_t*)sha1_state->block + SHA1_BLOCK_NB_BYTE - 8 + 1) = (uint8_t)(sha1_state->global_size >> 45);
	*((uint8_t*)sha1_state->block + SHA1_BLOCK_NB_BYTE - 8 + 2) = (uint8_t)(sha1_state->global_size >> 37);
	*((uint8_t*)sha1_state->block + SHA1_BLOCK_NB_BYTE - 8 + 3) = (uint8_t)(sha1_state->global_size >> 29);
	*((uint8_t*)sha1_state->block + SHA1_BLOCK_NB_BYTE - 8 + 4) = (uint8_t)(sha1_state->global_size >> 21);
	*((uint8_t*)sha1_state->block + SHA1_BLOCK_NB_BYTE - 8 + 5) = (uint8_t)(sha1_state->global_size >> 13);
	*((uint8_t*)sha1_state->block + SHA1_BLOCK_NB_BYTE - 8 + 6) = (uint8_t)(sha1_state->global_size >> 5);
	*((uint8_t*)sha1_state->block + SHA1_BLOCK_NB_BYTE - 8 + 7) = (uint8_t)(sha1_state->global_size << 3);

	sha1_compress(sha1_state);

	hash[0] = SHA1_STORE_DWORD(sha1_state->state[0]);
	hash[1] = SHA1_STORE_DWORD(sha1_state->state[1]);
	hash[2] = SHA1_STORE_DWORD(sha1_state->state[2]);
	hash[3] = SHA1_STORE_DWORD(sha1_state->state[3]);
	hash[4] = SHA1_STORE_DWORD(sha1_state->state[4]);
}
