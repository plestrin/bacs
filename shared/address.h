#ifndef ADDRESS_H
#define ADDRESS_H

#include <stdint.h>

#if defined ARCH_32
typedef uint32_t ADDRESS;
#define ADDRESS_SIZE 32
#define PRINTF_ADDR "0x%08x"
#define PRINTF_ADDR_SHORT "0x%x"
#elif defined ARCH_64
typedef uint64_t ADDRESS;
#define ADDRESS_SIZE 64
#define PRINTF_ADDR "0x%llx"
#define PRINTF_ADDR_SHORT PRINTF_ADDR
#else
#error Please specify an architecture {ARCH_32 or ARCH_64}
#endif

#endif
