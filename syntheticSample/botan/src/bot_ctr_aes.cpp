#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <botan/init.h>
#include <botan/pipe.h>
#include <botan/ctr.h>
#include <botan/aes.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

static void botan_patch();
static void botan_ctr_aes();

int main() {
	botan_patch();
	botan_ctr_aes();
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

static void botan_ctr_aes(){
	char 						pt[] = "Hi I am an AES CTR test vector distributed on 4 128-bit blocks!";
	unsigned char 				key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 				iv[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	Botan::byte 				ct[sizeof(pt)];
	Botan::byte					vt[sizeof(pt)];
	Botan::CTR_BE* 				ctr_aes;

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "IV:             ";
	print_raw_buffer((Botan::byte*)iv, 16);
	std::cout << std::endl << "Key 128:        ";
	print_raw_buffer((Botan::byte*)key, 16);

	ctr_aes = new Botan::CTR_BE(new Botan::AES_128);
	ctr_aes->set_key(key, sizeof(key));
	ctr_aes->set_iv(iv, sizeof(iv));
	ctr_aes->cipher((Botan::byte*)pt, ct, sizeof(pt));

	ctr_aes->clear();

	ctr_aes->set_key(key, sizeof(key));
	ctr_aes->set_iv(iv, sizeof(iv));
	ctr_aes->cipher(ct, vt, sizeof(pt));

	delete(ctr_aes);

	if (!memcmp(pt, vt, 16)){
		std::cout << std::endl << "Ciphertext CTR: ";
		print_raw_buffer(ct, sizeof(pt));
		std::cout << std::endl << "Recovery:       OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext CTR: ";
		print_raw_buffer(ct, sizeof(pt));
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
	}
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