#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <cryptopp/modes.h>
#include <cryptopp/tea.h>

static void print_raw_buffer(byte* buffer, size_t buffer_length){
	size_t i;

	for (i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}

static void readBuffer_reverse_endianness(byte* buffer, size_t buffer_length){
	size_t i;

	if (buffer_length % 4){
		std::cout << "ERROR: buffer size (in byte) must be a multiple of 4" << std::endl;
		return;
	}

	for (i = 0; i < buffer_length; i += 4){
		*(uint32_t*)(buffer + i) = (*(uint32_t*)(buffer + i) >> 24) | ((*(uint32_t*)(buffer + i) >> 8) & 0x0000ff00) | ((*(uint32_t*)(buffer + i) << 8) & 0x00ff0000) | (*(uint32_t*)(buffer + i) << 24);
	}
}

int main(void){
	unsigned char 	key[16]	= {0x12, 0x45, 0xf0, 0x6a, 0x45, 0x89, 0xfe, 0x60, 0x50, 0xAA, 0x78, 0x59, 0xf5, 0x69, 0x41, 0xbb};
	unsigned char 	pt[] 	= {0x45, 0xb7, 0x28, 0xba, 0xd7, 0x8f, 0x1a, 0x1f};
	byte 			ct[sizeof pt];
	byte 			vt[sizeof ct];

	std::cout << "Plaintext:       ";
	print_raw_buffer((byte*)pt, sizeof pt);
	std::cout << std::endl << "Key:             ";
	print_raw_buffer((byte*)key, sizeof key);

	/* Crypto++ TEA seems to be Big endian */
	readBuffer_reverse_endianness((byte*)pt, sizeof pt);
	readBuffer_reverse_endianness((byte*)key, sizeof key);

	CryptoPP::ECB_Mode<CryptoPP::TEA>::Encryption ecb_enc_tea((byte*)key, sizeof key);
	ecb_enc_tea.ProcessData(ct, (byte*)pt, sizeof pt);

	CryptoPP::ECB_Mode<CryptoPP::TEA>::Decryption ecb_dec_tea((byte*)key, sizeof key);
	ecb_dec_tea.ProcessData(vt, ct, sizeof ct);

	std::cout << std::endl << "Ciphertext TEA:  ";
	readBuffer_reverse_endianness(ct, sizeof ct);
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:        FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:        OK" << std::endl;

	CryptoPP::ECB_Mode<CryptoPP::XTEA>::Encryption ecb_enc_xtea((byte*)key, sizeof key);
	ecb_enc_xtea.ProcessData(ct, (byte*)pt, sizeof pt);

	CryptoPP::ECB_Mode<CryptoPP::XTEA>::Decryption ecb_dec_xtea((byte*)key, sizeof key);
	ecb_dec_xtea.ProcessData(vt, ct, sizeof ct);

	std::cout << "Ciphertext XTEA: ";
	readBuffer_reverse_endianness(ct, sizeof ct);
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:        FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:        OK" << std::endl;

	return EXIT_SUCCESS;
}
