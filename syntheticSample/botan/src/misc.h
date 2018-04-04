#ifndef MISC_H
#define MISC_H

#include <botan/init.h>
#include <botan/pipe.h>

int botan_patch(void);

void print_raw_buffer(Botan::byte* buffer, unsigned int buffer_length);

void swap_endianness(Botan::byte* buffer, unsigned int buffer_length);

#endif
