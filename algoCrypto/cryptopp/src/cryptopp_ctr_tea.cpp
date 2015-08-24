#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <cryptopp/modes.h>
#include <cryptopp/tea.h>

void print_raw_buffer(byte* buffer, int buffer_length);
void readBuffer_reverse_endianness(byte* buffer, int buffer_length);

int main() {
	char 			pt[] = "Hi I am an XTEA CTR test vector distributed on 8 64-bit blocks!";
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[8] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4};
	char 			ct[sizeof(pt)];
	char 			vt[sizeof(pt)];

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "IV:             ";
	print_raw_buffer((byte*)iv, 8);
	std::cout << std::endl << "Key:            ";
	print_raw_buffer((byte*)key, 16);

	/* Crypto++ XTEA seems to be Big endian */
	readBuffer_reverse_endianness((byte*)pt, sizeof(pt));
	readBuffer_reverse_endianness((byte*)key, 16);
	readBuffer_reverse_endianness((byte*)iv, 8);

	CryptoPP::CTR_Mode<CryptoPP::XTEA>::Encryption ctr_enc_xtea((const byte*)key, 16, iv);
	ctr_enc_xtea.ProcessData((byte*)ct, (byte*)pt, sizeof(pt));

	CryptoPP::CTR_Mode<CryptoPP::XTEA>::Decryption ctr_dec_xtea((const byte*)key, 16, iv);
	ctr_dec_xtea.ProcessData((byte*)vt, (byte*)ct, sizeof(pt));

	if (!memcmp(pt, vt, sizeof(pt))){
		std::cout << std::endl << "Ciphertext CTR: ";
		readBuffer_reverse_endianness((byte*)ct, sizeof(pt));
		print_raw_buffer((byte*)ct, sizeof(pt));
		std::cout << std::endl << "Recovery:       OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext CTR: ";
		readBuffer_reverse_endianness((byte*)ct, sizeof(pt));
		print_raw_buffer((byte*)ct, sizeof(pt));
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
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