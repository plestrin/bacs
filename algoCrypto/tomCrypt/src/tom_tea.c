#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(){
	char* 			plaintext		= NULL;
	char* 			ciphertext 		= NULL;
	char* 			deciphertext 	= NULL;
	uint32_t		size 			= 32;
	uint32_t		key[4]	 		= {0x1245F06A, 0x4589FE60, 0x50AA7859, 0xF56941BB};
	symmetric_key 	skey;
	uint32_t 		i;
	
	plaintext 		= (char*)malloc(size);
	ciphertext 		= (char*)malloc(size);
	deciphertext 	= (char*)malloc(size);
	if (plaintext != NULL && ciphertext != NULL && deciphertext != NULL){
		memset(plaintext, 0, size);
		strncpy(plaintext, "Hello World!", size);
		
		printf("Plaintext:\t\"%s\"\n", plaintext);
		printf("Key: \t\t");
		printBuffer_raw(stdout, (char*)key, 16);
		printf("\n");

		if (xtea_setup((const unsigned char*)key, 16, 32, &skey) != CRYPT_OK){
			printf("ERROR: unable to setup xtea key\n");
			return -1;
		}

		for (i = 0; i < size; i+= 8){
			if (xtea_ecb_encrypt((const unsigned char*)plaintext + i, (unsigned char *)ciphertext + i, &skey) != CRYPT_OK){
				printf("ERROR: unable to encrypt xtea\n");
				break;
			}
		}

		for (i = 0; i < size; i+= 8){
			if (xtea_ecb_decrypt((const unsigned char*)ciphertext + i, (unsigned char *)deciphertext + i, &skey) != CRYPT_OK){
				printf("ERROR: unable to decrypt xtea\n");
				return -1;
			}
		}

		printf("Ciphertext XTEA:");
		printBuffer_raw(stdout, ciphertext, size);
		
		if (memcmp(plaintext, deciphertext, size) == 0){
			printf("\nRecovery XTEA: \tOK\n");
		}
		else{
			printf("\nRecovery XTEA: \tFAIL\n");
		}


		free(plaintext);
		free(ciphertext);
		free(deciphertext);
	}
	else{
		printf("ERROR: unable to allocate memory\n");
	}

	return 0;
}
