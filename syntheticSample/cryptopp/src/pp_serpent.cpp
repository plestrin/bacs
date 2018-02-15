#include <iostream>
#include <iomanip>

#include <cryptopp/modes.h>
#include <cryptopp/serpent.h>

#define KEY_SIZE 24

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

int main(void){
	byte	key[KEY_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	byte	pt[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	byte	ct[sizeof pt];
	byte	vt[sizeof ct];

	CryptoPP::ECB_Mode<CryptoPP::Serpent>::Encryption ecb_enc_serpent(key, KEY_SIZE);
	ecb_enc_serpent.ProcessData(ct, pt, sizeof pt);

	std::cout << "Plaintext:  ";
	print_raw_buffer(pt, sizeof pt);
	std::cout << std::endl << "Key:        ";
	print_raw_buffer(key, KEY_SIZE);
	std::cout << std::endl << "Ciphertext: ";
	print_raw_buffer(ct, sizeof ct);

	CryptoPP::ECB_Mode<CryptoPP::Serpent>::Decryption ecb_dec_serpent(key, KEY_SIZE);
	ecb_dec_serpent.ProcessData(vt, ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:   OK" << std::endl;

	return EXIT_SUCCESS;
}
