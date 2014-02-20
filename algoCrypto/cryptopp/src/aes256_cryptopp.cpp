#include <iostream>
#include <iomanip>

#include <cryptopp/modes.h>
#include <cryptopp/aes.h>

void print_raw_buffer(byte* buffer, int buffer_length);

int main(int argc, char* argv[]) {
	byte	key256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
	byte	pt[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	byte	ct[16];
	byte	vt[16];

	CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption ecb_enc_aes256(key256, 32);
	ecb_enc_aes256.ProcessData(ct, pt, 16);

	std::cout << "Plaintext:      ";
	print_raw_buffer(pt, 16);
	std::cout << std::endl << "Key 256:        ";
	print_raw_buffer(key256, 32);
	std::cout << std::endl << "Ciphertext 256: ";
	print_raw_buffer(ct, 16);

	CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption ecb_dec_aes256(key256, 32);
	ecb_dec_aes256.ProcessData(vt, ct, 16);

	if (!memcmp(pt, vt, 16)){
		std::cout << std::endl << "Recovery 256:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Recovery 256:   FAIL" << std::endl;
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