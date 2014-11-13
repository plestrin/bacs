#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <botan/init.h>
#include <botan/des.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

int main() {
	unsigned char key[8] = {0x75, 0x29, 0x79, 0x38, 0x75, 0x92, 0xcb, 0x70};
	
	unsigned char plaintext[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	unsigned char ciphertext[8];
	unsigned char deciphertext[8];

	Botan::DES 	des;

	std::cout << "Plaintext:   ";
	print_raw_buffer((Botan::byte*)plaintext, 8);

	std::cout << std::endl << "Key:         ";
	print_raw_buffer((Botan::byte*)key, 8);

	des.set_key((Botan::byte*)key, 8);
	des.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	des.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 8)){
		std::cout << std::endl << "Ciphertext : ";
		print_raw_buffer((Botan::byte*)ciphertext, 8);
		std::cout << std::endl << "Recovery :   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext : ";
		print_raw_buffer((Botan::byte*)ciphertext, 8);
		std::cout << std::endl << "Recovery :   FAIL" << std::endl;
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