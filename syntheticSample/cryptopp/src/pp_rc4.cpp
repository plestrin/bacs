#include <iostream>
#include <iomanip>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <cryptopp/arc4.h>

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
	byte 					pt[] = "Hello World!";
	byte					key[] = "Key";
	byte 					ct[sizeof pt];
	CryptoPP::Weak::ARC4* 	rc4;

	std::cout << "Plaintext:  \"" << pt << "\"" << std::endl;
	std::cout << "Key:        \"" << key << "\"" << std::endl;

	rc4 = new CryptoPP::Weak::ARC4(key, strlen((char*)key));
	rc4->ProcessData(ct, pt, strlen((char*)pt));

	std::cout << "Ciphertext: ";
	print_raw_buffer(ct, strlen((char*)pt));
	std::cout << std::endl;

	return EXIT_SUCCESS;
}
