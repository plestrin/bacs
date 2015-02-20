#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <botan/init.h>
#include <botan/arc4.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

int main() {
	unsigned char 	plaintext[] = "Hello World!";
	unsigned char 	key[] = "Key";
	unsigned char* 	ciphertext;
	unsigned char* 	deciphertext;
	Botan::ARC4 	rc4_enc;
	Botan::ARC4 	rc4_dec;
	unsigned int 	plaintext_length = sizeof(plaintext) - 1;

	ciphertext = (unsigned char*)malloc(plaintext_length);
	deciphertext = (unsigned char*)malloc(plaintext_length);

	if (ciphertext != NULL && deciphertext != NULL){
		std::cout << "Plaintext:   \"" << plaintext << "\"" << std::endl;
		std::cout << "Key:         \"" << key << "\"" << std::endl;

		rc4_enc.set_key((Botan::byte*)key, sizeof(key) - 1);
		rc4_enc.cipher((Botan::byte*)plaintext, (Botan::byte*)ciphertext, plaintext_length);

		rc4_dec.set_key((Botan::byte*)key, sizeof(key) - 1);
		rc4_dec.cipher((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, plaintext_length);

		if (!memcmp(plaintext, deciphertext, plaintext_length)){
			std::cout << "Ciphertext : ";
			print_raw_buffer((Botan::byte*)ciphertext, plaintext_length);
			std::cout << std::endl << "Recovery :   OK" << std::endl;
		}
		else{
			std::cout << "Ciphertext : ";
			print_raw_buffer((Botan::byte*)ciphertext, plaintext_length);
			std::cout << std::endl << "Recovery :   FAIL" << std::endl;
		}
	}

	return 0;
}

static void print_raw_buffer(Botan::byte* buffer, int buffer_length){
	for (int i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}