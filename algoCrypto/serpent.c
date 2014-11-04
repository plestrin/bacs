#include "serpent.h"

#define SERPENT_GOLDEN_RATIO 0x9e3779b9

#define ROTATE_L(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define ROTATE_R(x, n) (((x) >> (n)) | ((x) << (32 - (n))))


#define linear_enc(x0 ,x1, x2, x3)	\
	x0 = ROTATE_L(x0, 13); 			\
	x2 = ROTATE_L(x2, 3); 			\
	x1 = x1 ^ x0 ^ x2; 				\
	x3 = x3 ^ x2 ^ (x0 << 3); 		\
	x1 = ROTATE_L(x1, 1); 			\
	x3 = ROTATE_L(x3, 7); 			\
	x0 = x0 ^ x1 ^ x3; 				\
	x2 = x2 ^ x3 ^ (x1 << 7); 		\
	x0 = ROTATE_L(x0, 5); 			\
	x2 = ROTATE_L(x2, 22)

#define linear_dec(x0, x1, x2, x3) 	\
	x2 = ROTATE_R(x2, 22); 			\
	x0 = ROTATE_R(x0, 5); 			\
	x2 = x2 ^ x3 ^ (x1 << 7); 		\
	x0 = x0 ^ x1 ^ x3; 				\
	x3 = ROTATE_R(x3, 7); 			\
	x1 = ROTATE_R(x1, 1); 			\
	x3 = x3 ^x2 ^ (x0 << 3); 		\
	x1 =  x1 ^ x0 ^ x2; 			\
	x2 = ROTATE_R(x2, 3); 			\
	x0 = ROTATE_R(x0, 13)

#define SBoxE0(x0, x1, x2, x3)		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T3 ^= T0; 					\
		T4 = T1; 					\
		T1 &= T3; 					\
		T4 ^= T2; 					\
		T1 ^= T0; 					\
		T0 |= T3; 					\
		T0 ^= T4; 					\
		T4 ^= T3; 					\
		T3 ^= T2; 					\
		T2 |= T1; 					\
		T2 ^= T4; 					\
		T4 = ~T4; 					\
		T4 |= T1; 					\
		T1 ^= T3; 					\
		T1 ^= T4; 					\
		T3 |= T0; 					\
		T1 ^= T3; 					\
		T4 ^= T3; 					\
 									\
		x0 = T1; 					\
		x1 = T4; 					\
		x2 = T2; 					\
		x3 = T0; 					\
	}

#define SBoxE1(x0, x1, x2, x3)		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T0 = ~T0;					\
		T2 = ~T2;					\
		T4  = T0;					\
		T0 &= T1;					\
		T2 ^= T0;					\
		T0 |= T3;					\
		T3 ^= T2;					\
		T1 ^= T0;					\
		T0 ^= T4;					\
		T4 |= T1;					\
		T1 ^= T3;					\
		T2 |= T0;					\
		T2 &= T4;					\
		T0 ^= T1;					\
		T1 &= T2;					\
		T1 ^= T0;					\
		T0 &= T2;					\
		T0 ^= T4;					\
									\
		x0 = T2; 					\
		x1 = T0; 					\
		x2 = T3; 					\
		x3 = T1; 					\
	}

#define SBoxE2(x0, x1, x2, x3)		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T4  = T0; 					\
		T0 &= T2; 					\
		T0 ^= T3; 					\
		T2 ^= T1; 					\
		T2 ^= T0; 					\
		T3 |= T4; 					\
		T3 ^= T1; 					\
		T4 ^= T2; 					\
		T1  = T3; 					\
		T3 |= T4; 					\
		T3 ^= T0; 					\
		T0 &= T1; 					\
		T4 ^= T0; 					\
		T1 ^= T3; 					\
		T1 ^= T4; 					\
		T4 = ~T4; 					\
									\
		x0 = T2;					\
		x1 = T3;					\
		x2 = T1;					\
		x3 = T4;					\
	}

#define SBoxE3(x0, x1, x2, x3)		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T4  = T0; 					\
		T0 |= T3; 					\
		T3 ^= T1; 					\
		T1 &= T4; 					\
		T4 ^= T2; 					\
		T2 ^= T3; 					\
		T3 &= T0; 					\
		T4 |= T1; 					\
		T3 ^= T4; 					\
		T0 ^= T1; 					\
		T4 &= T0; 					\
		T1 ^= T3; 					\
		T4 ^= T2; 					\
		T1 |= T0; 					\
		T1 ^= T2; 					\
		T0 ^= T3; 					\
		T2  = T1; 					\
		T1 |= T3; 					\
		T1 ^= T0; 					\
									\
		x0 = T1; 					\
		x1 = T2; 					\
		x2 = T3; 					\
		x3 = T4; 					\
	}

#define SBoxE4(x0, x1, x2, x3)		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T1 ^= T3; 					\
		T3 = ~T3; 					\
		T2 ^= T3; 					\
		T3 ^= T0; 					\
		T4  = T1; 					\
		T1 &= T3; 					\
		T1 ^= T2; 					\
		T4 ^= T3; 					\
		T0 ^= T4; 					\
		T2 &= T4; 					\
		T2 ^= T0; 					\
		T0 &= T1; 					\
		T3 ^= T0; 					\
		T4 |= T1; 					\
		T4 ^= T0; 					\
		T0 |= T3; 					\
		T0 ^= T2; 					\
		T2 &= T3; 					\
		T0 = ~T0; 					\
		T4 ^= T2; 					\
									\
		x0 = T1; 					\
		x1 = T4; 					\
		x2 = T0; 					\
		x3 = T3; 					\
	}

#define SBoxE5(x0, x1, x2, x3)		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T0 ^= T1; 					\
		T1 ^= T3; 					\
		T3 = ~T3; 					\
		T4  = T1; 					\
		T1 &= T0; 					\
		T2 ^= T3; 					\
		T1 ^= T2; 					\
		T2 |= T4; 					\
		T4 ^= T3; 					\
		T3 &= T1; 					\
		T3 ^= T0; 					\
		T4 ^= T1; 					\
		T4 ^= T2; 					\
		T2 ^= T0; 					\
		T0 &= T3; 					\
		T2 = ~T2; 					\
		T0 ^= T4; 					\
		T4 |= T3; 					\
		T2 ^= T4; 					\
									\
		x0 = T1; 					\
		x1 = T3; 					\
		x2 = T0; 					\
		x3 = T2; 					\
	}

#define SBoxE6(x0, x1, x2, x3)		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T2 = ~T2; 					\
		T4  = T3; 					\
		T3 &= T0; 					\
		T0 ^= T4; 					\
		T3 ^= T2; 					\
		T2 |= T4; 					\
		T1 ^= T3; 					\
		T2 ^= T0; 					\
		T0 |= T1; 					\
		T2 ^= T1; 					\
		T4 ^= T0; 					\
		T0 |= T3; 					\
		T0 ^= T2; 					\
		T4 ^= T3; 					\
		T4 ^= T0; 					\
		T3 = ~T3; 					\
		T2 &= T4; 					\
		T2 ^= T3; 					\
									\
		x0 = T0; 					\
		x1 = T1; 					\
		x2 = T4; 					\
		x3 = T2; 					\
	}

#define SBoxE7(x0, x1, x2, x3)		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T4  = T1; 					\
		T1 |= T2; 					\
		T1 ^= T3; 					\
		T4 ^= T2; 					\
		T2 ^= T1; 					\
		T3 |= T4; 					\
		T3 &= T0; 					\
		T4 ^= T2; 					\
		T3 ^= T1; 					\
		T1 |= T4; 					\
		T1 ^= T0; 					\
		T0 |= T4; 					\
		T0 ^= T2; 					\
		T1 ^= T4; 					\
		T2 ^= T1; 					\
		T1 &= T0; 					\
		T1 ^= T4; 					\
		T2 = ~T2; 					\
		T2 |= T0; 					\
		T4 ^= T2; 					\
									\
		x0 = T4; 					\
		x1 = T3; 					\
		x2 = T1; 					\
		x3 = T0; 					\
	}

#define SBoxD0(x0, x1, x2, x3) 		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T2 = ~T2; 					\
		T4  = T1; 					\
		T1 |= T0; 					\
		T4 = ~T4; 					\
		T1 ^= T2; 					\
		T2 |= T4; 					\
		T1 ^= T3; 					\
		T0 ^= T4; 					\
		T2 ^= T0; 					\
		T0 &= T3; 					\
		T4 ^= T0; 					\
		T0 |= T1; 					\
		T0 ^= T2; 					\
		T3 ^= T4; 					\
		T2 ^= T1; 					\
		T3 ^= T0; 					\
		T3 ^= T1; 					\
		T2 &= T3; 					\
		T4 ^= T2; 					\
									\
		x0 = T0; 					\
		x1 = T4; 					\
		x2 = T1; 					\
		x3 = T3; 					\
	}

#define SBoxD1(x0, x1, x2, x3) 		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T4  = T1; 					\
		T1 ^= T3; 					\
		T3 &= T1; 					\
		T4 ^= T2; 					\
		T3 ^= T0; 					\
		T0 |= T1; 					\
		T2 ^= T3; 					\
		T0 ^= T4; 					\
		T0 |= T2; 					\
		T1 ^= T3; 					\
		T0 ^= T1; 					\
		T1 |= T3; 					\
		T1 ^= T0; 					\
		T4 = ~T4; 					\
		T4 ^= T1; 					\
		T1 |= T0; 					\
		T1 ^= T0; 					\
		T1 |= T4; 					\
		T3 ^= T1; 					\
									\
		x0 = T4; 					\
		x1 = T0; 					\
		x2 = T3; 					\
		x3 = T2; 					\
	}

#define SBoxD2(x0, x1, x2, x3) 		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T2 ^= T3; 					\
		T3 ^= T0; 					\
		T4  = T3; 					\
		T3 &= T2; 					\
		T3 ^= T1; 					\
		T1 |= T2; 					\
		T1 ^= T4; 					\
		T4 &= T3; 					\
		T2 ^= T3; 					\
		T4 &= T0; 					\
		T4 ^= T2; 					\
		T2 &= T1; 					\
		T2 |= T0; 					\
		T3 = ~T3; 					\
		T2 ^= T3; 					\
		T0 ^= T3; 					\
		T0 &= T1; 					\
		T3 ^= T4; 					\
		T3 ^= T0; 					\
									\
		x0 = T1; 					\
		x1 = T4; 					\
		x2 = T2; 					\
		x3 = T3; 					\
	}

#define SBoxD3(x0, x1, x2, x3) 		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T4  = T2; 					\
		T2 ^= T1; 					\
		T0 ^= T2; 					\
		T4 &= T2; 					\
		T4 ^= T0; 					\
		T0 &= T1; 					\
		T1 ^= T3; 					\
		T3 |= T4; 					\
		T2 ^= T3; 					\
		T0 ^= T3; 					\
		T1 ^= T4; 					\
		T3 &= T2; 					\
		T3 ^= T1; 					\
		T1 ^= T0; 					\
		T1 |= T2; 					\
		T0 ^= T3; 					\
		T1 ^= T4; 					\
		T0 ^= T1; 					\
									\
		x0 = T2; 					\
		x1 = T1; 					\
		x2 = T3; 					\
		x3 = T0; 					\
	}

#define SBoxD4(x0, x1, x2, x3) 		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T4  = T2; 					\
		T2 &= T3; 					\
		T2 ^= T1; 					\
		T1 |= T3; 					\
		T1 &= T0; 					\
		T4 ^= T2; 					\
		T4 ^= T1; 					\
		T1 &= T2; 					\
		T0 = ~T0; 					\
		T3 ^= T4; 					\
		T1 ^= T3; 					\
		T3 &= T0; 					\
		T3 ^= T2; 					\
		T0 ^= T1; 					\
		T2 &= T0; 					\
		T3 ^= T0; 					\
		T2 ^= T4; 					\
		T2 |= T3; 					\
		T3 ^= T0; 					\
		T2 ^= T1; 					\
									\
		x0 = T0; 					\
		x1 = T3; 					\
		x2 = T2; 					\
		x3 = T4; 					\
	}

#define SBoxD5(x0, x1, x2, x3) 		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T1 = ~T1; 					\
		T4  = T3; 					\
		T2 ^= T1; 					\
		T3 |= T0; 					\
		T3 ^= T2; 					\
		T2 |= T1; 					\
		T2 &= T0; 					\
		T4 ^= T3; 					\
		T2 ^= T4; 					\
		T4 |= T0; 					\
		T4 ^= T1; 					\
		T1 &= T2; 					\
		T1 ^= T3; 					\
		T4 ^= T2; 					\
		T3 &= T4; 					\
		T4 ^= T1; 					\
		T3 ^= T4; 					\
		T4 = ~T4; 					\
		T3 ^= T0; 					\
									\
		x0 = T1; 					\
		x1 = T4; 					\
		x2 = T3; 					\
		x3 = T2; 					\
	}

#define SBoxD6(x0, x1, x2, x3) 		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T0 ^= T2; 					\
		T4  = T2; 					\
		T2 &= T0; 					\
		T4 ^= T3; 					\
		T2 = ~T2; 					\
		T3 ^= T1; 					\
		T2 ^= T3; 					\
		T4 |= T0; 					\
		T0 ^= T2; 					\
		T3 ^= T4; 					\
		T4 ^= T1; 					\
		T1 &= T3; 					\
		T1 ^= T0; 					\
		T0 ^= T3; 					\
		T0 |= T2; 					\
		T3 ^= T1; 					\
		T4 ^= T0; 					\
									\
		x0 = T1; 					\
		x1 = T2; 					\
		x2 = T4; 					\
		x3 = T3; 					\
	}

#define SBoxD7(x0, x1, x2, x3) 		\
	{ 								\
		uint32_t T0 = x0; 			\
		uint32_t T1 = x1; 			\
		uint32_t T2 = x2; 			\
		uint32_t T3 = x3; 			\
		uint32_t T4; 				\
									\
		T4  = T2; 					\
		T2 ^= T0; 					\
		T0 &= T3; 					\
		T4 |= T3; 					\
		T2 = ~T2; 					\
		T3 ^= T1; 					\
		T1 |= T0; 					\
		T0 ^= T2; 					\
		T2 &= T4; 					\
		T3 &= T4; 					\
		T1 ^= T2; 					\
		T2 ^= T0; 					\
		T0 |= T2; 					\
		T4 ^= T1; 					\
		T0 ^= T3; 					\
		T3 ^= T4; 					\
		T4 |= T0; 					\
		T3 ^= T2; 					\
		T4 ^= T2; 					\
									\
		x0 = T3; 					\
		x1 = T0; 					\
		x2 = T1; 					\
		x3 = T4; 					\
	}

#define xor_key(x0, x1, x2, x3, round_key) 	\
	(x0) = (x0) ^ (round_key)[0]; 			\
	(x1) = (x1) ^ (round_key)[1]; 			\
	(x2) = (x2) ^ (round_key)[2]; 			\
	(x3) = (x3) ^ (round_key)[3]

void serpent_key_expand(uint32_t* key, uint32_t key_length, uint32_t* round_key){
	uint32_t i;

	if (key_length < SERPENT_KEY_MAX_NB_BIT){
		key[key_length >> 5] &= ~(0xffffffff << (key_length & 0x0000001f));
		key[key_length >> 5] |= 0x00000001 << (key_length & 0x0000001f);

		for (i = (key_length >> 5) + 1; i < SERPENT_KEY_MAX_NB_WORD; i++){
			key[i] = 0;
		}
	}

	round_key[0] = ROTATE_L(key[0] ^ key[3] ^ key[5] ^ key[7] ^ SERPENT_GOLDEN_RATIO ^ 0, 11);
	round_key[1] = ROTATE_L(key[1] ^ key[4] ^ key[6] ^ round_key[0] ^ SERPENT_GOLDEN_RATIO ^ 1, 11);
	round_key[2] = ROTATE_L(key[2] ^ key[5] ^ key[7] ^ round_key[1] ^ SERPENT_GOLDEN_RATIO ^ 2, 11);
	round_key[3] = ROTATE_L(key[3] ^ key[6] ^ round_key[0] ^ round_key[2] ^ SERPENT_GOLDEN_RATIO ^ 3, 11);
	round_key[4] = ROTATE_L(key[4] ^ key[7] ^ round_key[1] ^ round_key[3] ^ SERPENT_GOLDEN_RATIO ^ 4, 11);
	round_key[5] = ROTATE_L(key[5] ^ round_key[0] ^ round_key[2] ^ round_key[4] ^ SERPENT_GOLDEN_RATIO ^ 5, 11);
	round_key[6] = ROTATE_L(key[6] ^ round_key[1] ^ round_key[3] ^ round_key[5] ^ SERPENT_GOLDEN_RATIO ^ 6, 11);
	round_key[7] = ROTATE_L(key[7] ^ round_key[2] ^ round_key[4] ^ round_key[6] ^ SERPENT_GOLDEN_RATIO ^ 7, 11);

	for(i = 8; i < SERPENT_ROUND_KEY_NB_WORD; i++){
		round_key[i] = ROTATE_L(round_key[i - 8] ^ round_key[i - 5] ^ round_key[i - 3] ^ round_key[i - 1] ^ SERPENT_GOLDEN_RATIO ^ i, 11);
	}

	SBoxE3(round_key[0  ], round_key[1  ], round_key[2  ], round_key[3  ]);
	SBoxE2(round_key[4  ], round_key[5  ], round_key[6  ], round_key[7  ]);
	SBoxE1(round_key[8  ], round_key[9  ], round_key[10 ], round_key[11 ]);
	SBoxE0(round_key[12 ], round_key[13 ], round_key[14 ], round_key[15 ]);
	SBoxE7(round_key[16 ], round_key[17 ], round_key[18 ], round_key[19 ]);
	SBoxE6(round_key[20 ], round_key[21 ], round_key[22 ], round_key[23 ]);
	SBoxE5(round_key[24 ], round_key[25 ], round_key[26 ], round_key[27 ]);
	SBoxE4(round_key[28 ], round_key[29 ], round_key[30 ], round_key[31 ]);

	SBoxE3(round_key[32 ], round_key[33 ], round_key[34 ], round_key[35 ]);
	SBoxE2(round_key[36 ], round_key[37 ], round_key[38 ], round_key[39 ]);
	SBoxE1(round_key[40 ], round_key[41 ], round_key[42 ], round_key[43 ]);
	SBoxE0(round_key[44 ], round_key[45 ], round_key[46 ], round_key[47 ]);
	SBoxE7(round_key[48 ], round_key[49 ], round_key[50 ], round_key[51 ]);
	SBoxE6(round_key[52 ], round_key[53 ], round_key[54 ], round_key[55 ]);
	SBoxE5(round_key[56 ], round_key[57 ], round_key[58 ], round_key[59 ]);
	SBoxE4(round_key[60 ], round_key[61 ], round_key[62 ], round_key[63 ]);

	SBoxE3(round_key[64 ], round_key[65 ], round_key[66 ], round_key[67 ]);
	SBoxE2(round_key[68 ], round_key[69 ], round_key[70 ], round_key[71 ]);
	SBoxE1(round_key[72 ], round_key[73 ], round_key[74 ], round_key[75 ]);
	SBoxE0(round_key[76 ], round_key[77 ], round_key[78 ], round_key[79 ]);
	SBoxE7(round_key[80 ], round_key[81 ], round_key[82 ], round_key[83 ]);
	SBoxE6(round_key[84 ], round_key[85 ], round_key[86 ], round_key[87 ]);
	SBoxE5(round_key[88 ], round_key[89 ], round_key[90 ], round_key[91 ]);
	SBoxE4(round_key[92 ], round_key[93 ], round_key[94 ], round_key[95 ]);

	SBoxE3(round_key[96 ], round_key[97 ], round_key[98 ], round_key[99 ]);
	SBoxE2(round_key[100], round_key[101], round_key[102], round_key[103]);
	SBoxE1(round_key[104], round_key[105], round_key[106], round_key[107]);
	SBoxE0(round_key[108], round_key[109], round_key[110], round_key[111]);
	SBoxE7(round_key[112], round_key[113], round_key[114], round_key[115]);
	SBoxE6(round_key[116], round_key[117], round_key[118], round_key[119]);
	SBoxE5(round_key[120], round_key[121], round_key[122], round_key[123]);
	SBoxE4(round_key[124], round_key[125], round_key[126], round_key[127]);

	SBoxE3(round_key[128], round_key[129], round_key[130], round_key[131]);
}

void serpent_encrypt(uint32_t* input, uint32_t* round_key, uint32_t* output){
	uint32_t x0 = input[0];
	uint32_t x1 = input[1];
	uint32_t x2 = input[2];
	uint32_t x3 = input[3];

	xor_key(x0, x1, x2, x3, round_key + 0  ); SBoxE0(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 4  ); SBoxE1(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 8  ); SBoxE2(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 12 ); SBoxE3(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 16 ); SBoxE4(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 20 ); SBoxE5(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 24 ); SBoxE6(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 28 ); SBoxE7(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);

	xor_key(x0, x1, x2, x3, round_key + 32 ); SBoxE0(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 36 ); SBoxE1(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 40 ); SBoxE2(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 44 ); SBoxE3(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 48 ); SBoxE4(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 52 ); SBoxE5(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 56 ); SBoxE6(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 60 ); SBoxE7(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);

	xor_key(x0, x1, x2, x3, round_key + 64 ); SBoxE0(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 68 ); SBoxE1(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 72 ); SBoxE2(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 76 ); SBoxE3(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 80 ); SBoxE4(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 84 ); SBoxE5(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 88 ); SBoxE6(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 92 ); SBoxE7(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);

	xor_key(x0, x1, x2, x3, round_key + 96 ); SBoxE0(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 100); SBoxE1(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 104); SBoxE2(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 108); SBoxE3(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 112); SBoxE4(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 116); SBoxE5(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 120); SBoxE6(x0, x1, x2, x3); linear_enc(x0, x1, x2, x3);
	xor_key(x0, x1, x2, x3, round_key + 124); SBoxE7(x0, x1, x2, x3);

	output[0] = x0 ^ round_key[128];
	output[1] = x1 ^ round_key[129];
	output[2] = x2 ^ round_key[130];
	output[3] = x3 ^ round_key[131];
}

void serpent_decrypt(uint32_t* input, uint32_t* round_key, uint32_t* output){
	uint32_t x0 = input[0];
	uint32_t x1 = input[1];
	uint32_t x2 = input[2];
	uint32_t x3 = input[3];

	xor_key(x0, x1, x2, x3, round_key + 128); SBoxD7(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 124);
	linear_dec(x0, x1, x2, x3); SBoxD6(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 120);
	linear_dec(x0, x1, x2, x3); SBoxD5(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 116);
	linear_dec(x0, x1, x2, x3); SBoxD4(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 112);
	linear_dec(x0, x1, x2, x3); SBoxD3(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 108);
	linear_dec(x0, x1, x2, x3); SBoxD2(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 104);
	linear_dec(x0, x1, x2, x3); SBoxD1(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 100);
	linear_dec(x0, x1, x2, x3); SBoxD0(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 96 );

	linear_dec(x0, x1, x2, x3); SBoxD7(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 92 );
	linear_dec(x0, x1, x2, x3); SBoxD6(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 88 );
	linear_dec(x0, x1, x2, x3); SBoxD5(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 84 );
	linear_dec(x0, x1, x2, x3); SBoxD4(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 80 );
	linear_dec(x0, x1, x2, x3); SBoxD3(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 76 );
	linear_dec(x0, x1, x2, x3); SBoxD2(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 72 );
	linear_dec(x0, x1, x2, x3); SBoxD1(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 68 );
	linear_dec(x0, x1, x2, x3); SBoxD0(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 64 );

	linear_dec(x0, x1, x2, x3); SBoxD7(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 60 );
	linear_dec(x0, x1, x2, x3); SBoxD6(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 56 );
	linear_dec(x0, x1, x2, x3); SBoxD5(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 52 );
	linear_dec(x0, x1, x2, x3); SBoxD4(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 48 );
	linear_dec(x0, x1, x2, x3); SBoxD3(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 44 );
	linear_dec(x0, x1, x2, x3); SBoxD2(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 40 );
	linear_dec(x0, x1, x2, x3); SBoxD1(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 36 );
	linear_dec(x0, x1, x2, x3); SBoxD0(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 32 );

	linear_dec(x0, x1, x2, x3); SBoxD7(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 28 );
	linear_dec(x0, x1, x2, x3); SBoxD6(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 24 );
	linear_dec(x0, x1, x2, x3); SBoxD5(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 20 );
	linear_dec(x0, x1, x2, x3); SBoxD4(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 16 );
	linear_dec(x0, x1, x2, x3); SBoxD3(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 12 );
	linear_dec(x0, x1, x2, x3); SBoxD2(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 8  );
	linear_dec(x0, x1, x2, x3); SBoxD1(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 4  );
	linear_dec(x0, x1, x2, x3); SBoxD0(x0, x1, x2, x3); xor_key(x0, x1, x2, x3, round_key + 0  );

	output[0] = x0;
	output[1] = x1;
	output[2] = x2;
	output[3] = x3;
}