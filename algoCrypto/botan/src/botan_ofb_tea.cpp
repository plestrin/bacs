#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <botan/init.h>
#include <botan/filters.h>
#include <botan/ofb.h>
#include <botan/xtea.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);
static void readBuffer_reverse_endianness(Botan::byte* buffer, int buffer_length);

static void botan_patch();
static void botan_ofb_tea();

int main() {
	botan_patch();
	botan_ofb_tea();
	return 0;
}

static void botan_patch(){
	void* 	lib_handle;
	char* 	func_addr;
	int32_t page_size;

	lib_handle = dlopen("/usr/lib/libbotan-1.10.so.0", RTLD_LAZY);
	if (lib_handle == NULL){
		std::cout << "ERROR: in " << __func__ << ", " << dlerror() << std::endl;
		return;
	}

	func_addr = (char*)dlsym(lib_handle, "_ZN5Botan26confirm_startup_self_testsERNS_17Algorithm_FactoryE");
	if (func_addr == NULL){
		std::cout << "ERROR: in " << __func__ << ", " << dlerror() << std::endl;
		goto exit;
	}

	page_size = getpagesize();

	if (mprotect(func_addr - ((unsigned long)func_addr % page_size), page_size, PROT_EXEC | PROT_READ | PROT_WRITE)){
		std::cout << "ERROR: in " << __func__ << ", unable to set memory write permission" << std::endl;
		goto exit;
	}

	*func_addr = 0xc3;

	if (mprotect(func_addr - ((long)func_addr % page_size), page_size, PROT_EXEC | PROT_READ)){
		std::cout << "ERROR: in " << __func__ << ", unable to reset memory permission" << std::endl;
		goto exit;
	}

	exit:
	if (dlclose(lib_handle)){
		std::cout << "ERROR: in " << __func__ << ", " << dlerror() << std::endl;
	}
}

static void botan_ofb_tea(){
	char 						pt[] = "Hi I am an XTEA OFB test vector distributed on 8 64-bit blocks!";
	unsigned char 				key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 				iv[8] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4};
	Botan::byte 				ct[sizeof(pt)];
	Botan::byte					vt[sizeof(pt)];
	Botan::XTEA* 				tea;
	Botan::StreamCipher_Filter* enc_ofb_tea;
	Botan::Pipe* 				enc_pipe;
	Botan::StreamCipher_Filter* dec_ofb_tea;
	Botan::Pipe* 				dec_pipe;

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "IV:             ";
	print_raw_buffer((Botan::byte*)iv, 8);
	std::cout << std::endl << "Key:            ";
	print_raw_buffer((Botan::byte*)key, 16);

	readBuffer_reverse_endianness((Botan::byte*)pt, sizeof(pt));
	readBuffer_reverse_endianness((Botan::byte*)key, 16);
	readBuffer_reverse_endianness((Botan::byte*)iv, 8);

	Botan::SymmetricKey 		botan_key(key, 16);
	Botan::InitializationVector botan_iv(iv, 8);

	tea = new Botan::XTEA();
	enc_ofb_tea = new Botan::StreamCipher_Filter(new Botan::OFB(tea->clone()), botan_key);
	enc_ofb_tea->set_iv(botan_iv);
	enc_pipe = new Botan::Pipe(enc_ofb_tea, NULL, NULL);

	enc_pipe->process_msg((Botan::byte*)pt, sizeof(pt));
	if (enc_pipe->read(ct, sizeof(pt)) != sizeof(pt)){
		std::cout << std::endl << "ERROR: the number of byte read from the pipe is incorrect" << std::endl;
		return;
	}

	delete(enc_pipe);

	dec_ofb_tea = new Botan::StreamCipher_Filter(new Botan::OFB(tea->clone()), botan_key);
	dec_ofb_tea->set_iv(botan_iv);
	dec_pipe = new Botan::Pipe(dec_ofb_tea, NULL, NULL);

	dec_pipe->process_msg(ct, sizeof(pt));
	if (dec_pipe->read(vt, sizeof(pt)) != sizeof(pt)){
		std::cout << std::endl << "ERROR: the number of byte read from the pipe is incorrect" << std::endl;
		return;
	}

	delete(dec_pipe);

	if (!memcmp(pt, vt, 16)){
		readBuffer_reverse_endianness((Botan::byte*)ct, sizeof(pt));

		std::cout << std::endl << "Ciphertext OFB: ";
		print_raw_buffer(ct, sizeof(pt));
		std::cout << std::endl << "Recovery:       OK" << std::endl;
	}
	else{
		readBuffer_reverse_endianness((Botan::byte*)ct, sizeof(pt));

		std::cout << std::endl << "Ciphertext OFB: ";
		print_raw_buffer(ct, sizeof(pt));
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
	}

	delete(tea);
}

static void print_raw_buffer(Botan::byte* buffer, int buffer_length){
	for (int i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}

static void readBuffer_reverse_endianness(Botan::byte* buffer, int buffer_length){
	int i;

	if (buffer_length % 4){
		std::cout << "ERROR: buffer size (in byte) must be a multiple of 4" << std::endl;
		return;
	}

	for (i = 0; i < buffer_length; i += 4){
		*(uint32_t*)(buffer + i) = (*(uint32_t*)(buffer + i) >> 24) | ((*(uint32_t*)(buffer + i) >> 8) & 0x0000ff00) | ((*(uint32_t*)(buffer + i) << 8) & 0x00ff0000) | (*(uint32_t*)(buffer + i) << 24);
	}
}