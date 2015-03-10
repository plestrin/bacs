#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <botan/init.h>
#include <botan/tea.h>
#include <botan/xtea.h>

void print_raw_buffer(Botan::byte* buffer, int buffer_length);
void readBuffer_reverse_endianness(Botan::byte* buffer, int buffer_length);

static void botan_patch();
static void botan_tea_xtea();

int main() {
	botan_patch();
	botan_tea_xtea();
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

static void botan_tea_xtea() {
	uint32_t		key[4] = {0x1245F06A, 0x4589FE60, 0x50AA7859, 0xF56941BB};
	char* 			pt;
	char* 			ct;
	char* 			vt;
	uint32_t		size = 32;
	Botan::TEA 		tea;
	Botan::XTEA 	xtea;

	pt 	= (char*)malloc(size);
	ct 	= (char*)malloc(size);
	vt 	= (char*)malloc(size);
	if (pt == NULL || ct == NULL || vt == NULL){
		std::cout << "ERROR: unable to allocate memory" << std::endl;
		return;
	}

	memset(pt, 0, size);
	strncpy(pt, "Hello World!", size);

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl << "Key: \t\t";
	print_raw_buffer((Botan::byte*)key, 16);

	/* Botan TEA seems to be Big endian */
	readBuffer_reverse_endianness((Botan::byte*)pt, size);
	readBuffer_reverse_endianness((Botan::byte*)key, 16);

	tea.set_key((Botan::byte*)key, 16);
	tea.encrypt_n((Botan::byte*)pt, (Botan::byte*)ct, 4);
	tea.decrypt_n((Botan::byte*)ct, (Botan::byte*)vt, 4);

	if (!memcmp(pt, vt, size)){
		std::cout << std::endl << "Ciphertext TEA: ";
		readBuffer_reverse_endianness((Botan::byte*)ct, size);
		print_raw_buffer((Botan::byte*)ct, size);
		std::cout << std::endl << "Recovery TEA:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext TEA: ";
		readBuffer_reverse_endianness((Botan::byte*)ct, size);
		print_raw_buffer((Botan::byte*)ct, size);
		std::cout << std::endl << "Recovery TEA:   FAIL" << std::endl;
	}

	xtea.set_key((Botan::byte*)key, 16);
	xtea.encrypt_n((Botan::byte*)pt, (Botan::byte*)ct, 4);
	xtea.decrypt_n((Botan::byte*)ct, (Botan::byte*)vt, 4);

	if (!memcmp(pt, vt, size)){
		std::cout << "Ciphertext XTEA:";
		readBuffer_reverse_endianness((Botan::byte*)ct, size);
		print_raw_buffer((Botan::byte*)ct, size);
		std::cout << std::endl << "Recovery XTEA:  OK" << std::endl;
	}
	else{
		std::cout << "Ciphertext XTEA:";
		readBuffer_reverse_endianness((Botan::byte*)ct, size);
		print_raw_buffer((Botan::byte*)ct, size);
		std::cout << std::endl << "Recovery XTEA:  FAIL" << std::endl;
	}

	free(pt);
	free(ct);
	free(vt);
}

void print_raw_buffer(Botan::byte* buffer, int buffer_length){
	for (int i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}

void readBuffer_reverse_endianness(Botan::byte* buffer, int buffer_length){
	int i;

	if (buffer_length % 4){
		std::cout << "ERROR: buffer size (in byte) must be a multiple of 4" << std::endl;
		return;
	}

	for (i = 0; i < buffer_length; i += 4){
		*(uint32_t*)(buffer + i) = (*(uint32_t*)(buffer + i) >> 24) | ((*(uint32_t*)(buffer + i) >> 8) & 0x0000ff00) | ((*(uint32_t*)(buffer + i) << 8) & 0x00ff0000) | (*(uint32_t*)(buffer + i) << 24);
	}
}