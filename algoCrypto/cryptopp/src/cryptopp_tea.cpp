#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <cryptopp/modes.h>
#include <cryptopp/tea.h>

void print_raw_buffer(byte* buffer, int buffer_length);
void readBuffer_reverse_endianness(byte* buffer, int buffer_length);

int main() {
	uint32_t	key[4] = {0x1245F06A, 0x4589FE60, 0x50AA7859, 0xF56941BB};
	char* 		pt;
	char* 		ct;
	char* 		vt;
	uint32_t	size = 32;

	pt 	= (char*)malloc(size);
	ct 	= (char*)malloc(size);
	vt 	= (char*)malloc(size);
	if (pt == NULL || ct == NULL || vt == NULL){
		std::cout << "ERROR: unable to allocate memory" << std::endl;
		return 0;
	}

	memset(pt, 0, size);
	strncpy(pt, "Hello World!", size);

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl << "Key: \t\t";
	print_raw_buffer((byte*)key, 16);

	/* Crypto++ TEA seems to be Big endian */
	readBuffer_reverse_endianness((byte*)pt, size);
	readBuffer_reverse_endianness((byte*)key, 16);

	CryptoPP::ECB_Mode<CryptoPP::TEA>::Encryption ecb_enc_tea((const byte*)key, 16);
	ecb_enc_tea.ProcessData((byte*)ct, (byte*)pt, size);

	CryptoPP::ECB_Mode<CryptoPP::TEA>::Decryption ecb_dec_tea((const byte*)key, 16);
	ecb_dec_tea.ProcessData((byte*)vt, (byte*)ct, size);

	if (!memcmp(pt, vt, size)){
		std::cout << std::endl << "Ciphertext TEA: ";
		readBuffer_reverse_endianness((byte*)ct, size);
		print_raw_buffer((byte*)ct, size);
		std::cout << std::endl << "Recovery TEA:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext TEA: ";
		readBuffer_reverse_endianness((byte*)ct, size);
		print_raw_buffer((byte*)ct, size);
		std::cout << std::endl << "Recovery TEA:   FAIL" << std::endl;
	}

	CryptoPP::ECB_Mode<CryptoPP::XTEA>::Encryption ecb_enc_xtea((const byte*)key, 16);
	ecb_enc_xtea.ProcessData((byte*)ct, (byte*)pt, size);

	CryptoPP::ECB_Mode<CryptoPP::XTEA>::Decryption ecb_dec_xtea((const byte*)key, 16);
	ecb_dec_xtea.ProcessData((byte*)vt, (byte*)ct, size);

	if (!memcmp(pt, vt, size)){
		std::cout << "Ciphertext XTEA:";
		readBuffer_reverse_endianness((byte*)ct, size);
		print_raw_buffer((byte*)ct, size);
		std::cout << std::endl << "Recovery XTEA:  OK" << std::endl;
	}
	else{
		std::cout << "Ciphertext XTEA:";
		readBuffer_reverse_endianness((byte*)ct, size);
		print_raw_buffer((byte*)ct, size);
		std::cout << std::endl << "Recovery XTEA:  FAIL" << std::endl;
	}

	free(pt);
	free(ct);
	free(vt);

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