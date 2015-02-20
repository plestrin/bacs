#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <botan/init.h>
#include <botan/twofish.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

int main() {
	unsigned char key_128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char key_192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	unsigned char key_256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
	
	unsigned char plaintext[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	unsigned char ciphertext[16];
	unsigned char deciphertext[16];

	Botan::Twofish 	twofish128;
	Botan::Twofish 	twofish192;
	Botan::Twofish 	twofish256;

	std::cout << "Plaintext:      ";
	print_raw_buffer((Botan::byte*)plaintext, 16);

	/* 128 bits TWOFISH */
	std::cout << std::endl << "Key: \t\t";
	print_raw_buffer((Botan::byte*)key_128, 16);

	twofish128.set_key((Botan::byte*)key_128, 16);
	twofish128.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	twofish128.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 16)){
		std::cout << std::endl << "Ciphertext 128: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 128:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext 128: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 128:   FAIL" << std::endl;
	}

	/* 192 bits TWOFISH */
	std::cout << "Key: \t\t";
	print_raw_buffer((Botan::byte*)key_192, 24);

	twofish192.set_key((Botan::byte*)key_192, 24);
	twofish192.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	twofish192.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 16)){
		std::cout << std::endl << "Ciphertext 192: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 192:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext 192: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 192:   FAIL" << std::endl;
	}

	/* 256 bits TWOFISH */
	std::cout << "Key: \t\t";
	print_raw_buffer((Botan::byte*)key_256, 32);

	twofish256.set_key((Botan::byte*)key_256, 32);
	twofish256.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	twofish256.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 16)){
		std::cout << std::endl << "Ciphertext 256: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 256:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext 256: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 256:   FAIL" << std::endl;
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