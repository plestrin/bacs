#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#include <botan/selftest.h>

#include "misc.h"

int botan_patch(void){
	char* 	func_addr;
	int32_t page_size;

	func_addr = (char*)Botan::confirm_startup_self_tests;

	page_size = getpagesize();

	if (mprotect(func_addr - ((unsigned long)func_addr % page_size), page_size, PROT_EXEC | PROT_READ | PROT_WRITE)){
		std::cout << "ERROR: in " << __func__ << ", unable to set memory write permission" << std::endl;
		return -1;
	}

	*func_addr = 0xc3;

	if (mprotect(func_addr - ((long)func_addr % page_size), page_size, PROT_EXEC | PROT_READ)){
		std::cout << "ERROR: in " << __func__ << ", unable to reset memory permission" << std::endl;
		return -1;
	}

	return 0;
}

void print_raw_buffer(Botan::byte* buffer, unsigned int buffer_length){
	unsigned int i;

	for (i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << '0' << std::hex << (buffer[i] & 0xff);
		}
	}
}

void swap_endianness(Botan::byte* buffer, unsigned int buffer_length){
	unsigned int i;

	if (buffer_length % 4){
		std::cout << "ERROR: in " << __func__ << ", buffer size (in byte) must be a multiple of 4" << std::endl;
		return;
	}

	for (i = 0; i < buffer_length; i += 4){
		*(uint32_t*)(buffer + i) = (*(uint32_t*)(buffer + i) >> 24) | ((*(uint32_t*)(buffer + i) >> 8) & 0x0000ff00) | ((*(uint32_t*)(buffer + i) << 8) & 0x00ff0000) | (*(uint32_t*)(buffer + i) << 24);
	}
}
