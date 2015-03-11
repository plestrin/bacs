#include <iostream>
#include <iomanip>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <cryptopp/md5.h>

void print_raw_buffer(byte* buffer, int buffer_length);

int main(){
	char 				message[] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
	byte 				hash[CryptoPP::Weak::MD5::DIGESTSIZE];
	CryptoPP::Weak::MD5 md5;

	md5.CalculateDigest(hash, (unsigned char*)message, strlen(message));

	std::cout << "Plaintext: \"" << message << "\"" << std::endl;
	std::cout << "MD5 hash:  ";
	print_raw_buffer(hash, sizeof(hash));
	std::cout << std::endl;

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