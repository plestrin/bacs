#include <iostream>
#include <iomanip>

#include <cryptopp/modes.h>
#include <cryptopp/des.h>

void print_raw_buffer(byte* buffer, int buffer_length);

int main(int argc, char* argv[]) {
	byte	key[8] = {0x75, 0x29, 0x79, 0x38, 0x75, 0x92, 0xcb, 0x70};
	byte	pt[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	byte	ct[8];
	byte	vt[8];

	CryptoPP::ECB_Mode<CryptoPP::DES>::Encryption ecb_enc_des(key, 8);
	ecb_enc_des.ProcessData(ct, pt, 8);

	std::cout << "Plaintext:  ";
	print_raw_buffer(pt, 8);
	std::cout << std::endl << "Key:        ";
	print_raw_buffer(key, 8);
	std::cout << std::endl << "Ciphertext: ";
	print_raw_buffer(ct, 8);

	CryptoPP::ECB_Mode<CryptoPP::DES>::Decryption ecb_dec_des(key, 8);
	ecb_dec_des.ProcessData(vt, ct, 8);

	if (!memcmp(pt, vt, 8)){
		std::cout << std::endl << "Recovery:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Recovery:   FAIL" << std::endl;
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