#include <iostream>
#include <iomanip>

#include <cryptopp/modes.h>
#include <cryptopp/des.h>

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
	byte	key[8] = {0x75, 0x29, 0x79, 0x38, 0x75, 0x92, 0xcb, 0x70};
	byte	pt[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	byte	ct[sizeof pt];
	byte	vt[sizeof ct];

	CryptoPP::ECB_Mode<CryptoPP::DES>::Encryption ecb_enc_des(key, sizeof key);
	ecb_enc_des.ProcessData(ct, pt, sizeof pt);

	std::cout << "Plaintext:  ";
	print_raw_buffer(pt, sizeof pt);
	std::cout << std::endl << "Key:        ";
	print_raw_buffer(key, sizeof key);
	std::cout << std::endl << "Ciphertext: ";
	print_raw_buffer(ct, sizeof ct);

	CryptoPP::ECB_Mode<CryptoPP::DES>::Decryption ecb_dec_des(key, sizeof key);
	ecb_dec_des.ProcessData(vt, ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:   OK" << std::endl;

	return EXIT_SUCCESS;
}
