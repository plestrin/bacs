#include <stdlib.h>
#include <stdint.h>

#include "AES.h"
#include "mode.h"

int32_t main(void){
	uint32_t k1[AES_128_NB_DWORD_KEY] 		= {0xf00df00d, 0xf00df00d, 0xf00df00d, 0xf00df00d};
	uint32_t k2[AES_128_NB_DWORD_KEY] 		= {0xdeaddead, 0xdeaddead, 0xdeaddead, 0xdeaddead};
	uint32_t rk[AES_128_NB_DWORD_ROUND_KEY];
	uint32_t iv[AES_BLOCK_NB_DWORD];
	char pt[48] 							= "I am a 48-byte plaintext. Here is some padding.";
	char ct[48];


	aes128_key_expand_encrypt(k1, rk);
	aes128_encrypt(k2, rk, iv);

	aes128_key_expand_encrypt(k2, rk);
	mode_enc_cbc((blockCipher)aes128_encrypt, AES_BLOCK_NB_BYTE, (uint8_t*)pt, (uint8_t*)ct, sizeof(pt), rk, (uint8_t*)iv);

	return EXIT_SUCCESS;
}
