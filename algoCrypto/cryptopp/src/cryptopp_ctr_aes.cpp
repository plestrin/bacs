#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <cryptopp/modes.h>
#include <cryptopp/aes.h>

static void print_raw_buffer(byte* buffer, int buffer_length);

int main() {
	char 			pt[] = "Hi I am an AES CTR test vector distributed on 4 128-bit blocks!";
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	char 			ct[sizeof(pt)];
	char 			vt[sizeof(pt)];

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "IV:             ";
	print_raw_buffer((byte*)iv, sizeof(iv));
	std::cout << std::endl << "Key 128:        ";
	print_raw_buffer((byte*)key, sizeof(key));

	CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption ctr_enc_aes((const byte*)key, sizeof(key), iv);
	ctr_enc_aes.ProcessData((byte*)ct, (byte*)pt, sizeof(pt));

	CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption ctr_dec_aes((const byte*)key, sizeof(key), iv);
	ctr_dec_aes.ProcessData((byte*)vt, (byte*)ct, sizeof(pt));

	if (!memcmp(pt, vt, sizeof(pt))){
		std::cout << std::endl << "Ciphertext CTR: ";
		print_raw_buffer((byte*)ct, sizeof(pt));
		std::cout << std::endl << "Recovery:       OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext CTR: ";
		print_raw_buffer((byte*)ct, sizeof(pt));
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
	}

	return EXIT_SUCCESS;
}

static void print_raw_buffer(byte* buffer, int buffer_length){
	for (int i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}
