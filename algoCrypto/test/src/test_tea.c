#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "TEA.h"
#include "printBuffer.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

int main(){
	char* 			plaintext		= NULL;
	char* 			ciphertext 		= NULL;
	char* 			deciphertext 	= NULL;
	uint32_t		size 			= 32;
	uint32_t		key[4]	 		= {0x1245F06A, 0x4589FE60, 0x50AA7859, 0xF56941BB};
	
	plaintext 		= (char*)malloc(size);
	ciphertext 		= (char*)malloc(size);
	deciphertext 	= (char*)malloc(size);
	if (plaintext != NULL && ciphertext != NULL && deciphertext != NULL){
		memset(plaintext, 0, size);
		strncpy(plaintext, "Hello World!", size);
		
		printf("Plaintext:\t\"%s\"\n", plaintext);
		printf("Key: \t\t");
		printBuffer_raw(stdout, (char*)key, TEA_KEY_NB_BYTE);
		printf("\n");

		tea_encipher((uint32_t*)plaintext, (uint64_t)size, key, (uint32_t*)ciphertext);

		printf("Ciphertext: \t");
		printBuffer_raw(stdout, ciphertext, size);
		printf("\n");

		tea_decipher((uint32_t*)ciphertext, (uint64_t)size, key, (uint32_t*)deciphertext);
		
		if (memcmp(plaintext, deciphertext, size) == 0){
			printf("Recovery: \tOK\n");
		}
		else{
			printf("Recovery: \tFAIL\n");
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