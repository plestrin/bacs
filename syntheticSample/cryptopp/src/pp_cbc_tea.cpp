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
	char 			pt[] = "Hi I am an XTEA CBC test vector distributed on 8 64-bit blocks!";
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[8] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4};
	char 			ct[sizeof pt];
	char 			vt[sizeof pt];

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "IV:             ";
	print_raw_buffer((byte*)iv, sizeof iv);
	std::cout << std::endl << "Key:            ";
	print_raw_buffer((byte*)key, sizeof key);

	/* Crypto++ XTEA seems to be Big endian */
	readBuffer_reverse_endianness((byte*)pt, sizeof pt);
	readBuffer_reverse_endianness((byte*)key, sizeof key);
	readBuffer_reverse_endianness((byte*)iv, sizeof iv);

	CryptoPP::CBC_Mode<CryptoPP::XTEA>::Encryption cbc_enc_xtea((const byte*)key, 16, iv);
	cbc_enc_xtea.ProcessData((byte*)ct, (byte*)pt, sizeof pt);

	CryptoPP::CBC_Mode<CryptoPP::XTEA>::Decryption cbc_dec_xtea((const byte*)key, 16, iv);
	cbc_dec_xtea.ProcessData((byte*)vt, (byte*)ct, sizeof ct);

	std::cout << std::endl << "Ciphertext CBC: ";

	if (memcmp(pt, vt, sizeof pt)){
		readBuffer_reverse_endianness((byte*)ct, sizeof ct);
		print_raw_buffer((byte*)ct, sizeof ct);
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	readBuffer_reverse_endianness((byte*)ct, sizeof ct);
	print_raw_buffer((byte*)ct, sizeof ct);
	std::cout << std::endl << "Recovery:       OK" << std::endl;

	return EXIT_SUCCESS;
}
