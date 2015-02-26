#include <iostream>
#include <iomanip>

#include <cryptopp/sha.h>

void print_raw_buffer(byte* buffer, int buffer_length);

int main(){
	char 			message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	byte 			hash[CryptoPP::SHA1::DIGESTSIZE];
	CryptoPP::SHA1 	sha1;

	sha1.CalculateDigest(hash, (unsigned char*)message, strlen(message));

	std::cout << "Plaintext: \"" << message << "\"" << std::endl;
	std::cout << "SHA1 hash: ";
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