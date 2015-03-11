#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <cryptopp/modes.h>
#include <cryptopp/tea.h>

void print_raw_buffer(byte* buffer, int buffer_length);
void readBuffer_reverse_endianness(byte* buffer, int buffer_length);

int main() {
	unsigned char 	key[16]	= {0x12, 0x45, 0xf0, 0x6a, 0x45, 0x89, 0xfe, 0x60, 0x50, 0xAA, 0x78, 0x59, 0xf5, 0x69, 0x41, 0xbb};
	unsigned char 	pt[8] 	= {0x45, 0xb7, 0x28, 0xba, 0xd7, 0x8f, 0x1a, 0x1f};
	char 			ct[8];
	char 			vt[8];

	std::cout << "Plaintext:       ";
	print_raw_buffer((byte*)pt, 8);
	std::cout << std::endl << "Key:             ";
	print_raw_buffer((byte*)key, 16);

	/* Crypto++ TEA seems to be Big endian */
	readBuffer_reverse_endianness((byte*)pt, 8);
	readBuffer_reverse_endianness((byte*)key, 16);

	CryptoPP::ECB_Mode<CryptoPP::TEA>::Encryption ecb_enc_tea((const byte*)key, 16);
	ecb_enc_tea.ProcessData((byte*)ct, (byte*)pt, 8);

	CryptoPP::ECB_Mode<CryptoPP::TEA>::Decryption ecb_dec_tea((const byte*)key, 16);
	ecb_dec_tea.ProcessData((byte*)vt, (byte*)ct, 8);

	if (!memcmp(pt, vt, 8)){
		std::cout << std::endl << "Ciphertext TEA:  ";
		readBuffer_reverse_endianness((byte*)ct, 8);
		print_raw_buffer((byte*)ct, 8);
		std::cout << std::endl << "Recovery:        OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext TEA: ";
		readBuffer_reverse_endianness((byte*)ct, 8);
		print_raw_buffer((byte*)ct, 8);
		std::cout << std::endl << "Recovery:        FAIL" << std::endl;
	}

	CryptoPP::ECB_Mode<CryptoPP::XTEA>::Encryption ecb_enc_xtea((const byte*)key, 16);
	ecb_enc_xtea.ProcessData((byte*)ct, (byte*)pt, 8);

	CryptoPP::ECB_Mode<CryptoPP::XTEA>::Decryption ecb_dec_xtea((const byte*)key, 16);
	ecb_dec_xtea.ProcessData((byte*)vt, (byte*)ct, 8);

	if (!memcmp(pt, vt, 8)){
		std::cout << "Ciphertext XTEA: ";
		readBuffer_reverse_endianness((byte*)ct, 8);
		print_raw_buffer((byte*)ct, 8);
		std::cout << std::endl << "Recovery:        OK" << std::endl;
	}
	else{
		std::cout << "Ciphertext XTEA:";
		readBuffer_reverse_endianness((byte*)ct, 8);
		print_raw_buffer((byte*)ct, 8);
		std::cout << std::endl << "Recovery:        FAIL" << std::endl;
	}

	return 0;
}

void print_raw_buffer(byte* buffer, int buffer_length){
	for (int i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}

void readBuffer_reverse_endianness(byte* buffer, int buffer_length){
	int i;

	if (buffer_length % 4){
		std::cout << "ERROR: buffer size (in byte) must be a multiple of 4" << std::endl;
		return;
	}

	for (i = 0; i < buffer_length; i += 4){
		*(uint32_t*)(buffer + i) = (*(uint32_t*)(buffer + i) >> 24) | ((*(uint32_t*)(buffer + i) >> 8) & 0x0000ff00) | ((*(uint32_t*)(buffer + i) << 8) & 0x00ff0000) | (*(uint32_t*)(buffer + i) << 24);
	}
}