#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <botan/init.h>
#include <botan/serpent.h>

#define KEY_SIZE 24

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

int main() {
	unsigned char key[KEY_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	
	unsigned char plaintext[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	unsigned char ciphertext[16];
	unsigned char deciphertext[16];

	Botan::Serpent 	serpent;

	std::cout << "Plaintext:          ";
	print_raw_buffer((Botan::byte*)plaintext, 16);

	std::cout << std::endl << "Key:                ";
	print_raw_buffer((Botan::byte*)key, KEY_SIZE);

	serpent.set_key((Botan::byte*)key, KEY_SIZE);
	serpent.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	serpent.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 16)){
		std::cout << std::endl << "Ciphertext Serpent: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery Serpent:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext Serpent: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery Serpent:   FAIL" << std::endl;
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