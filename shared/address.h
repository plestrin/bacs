#ifndef ADDRESS_H
#define ADDRESS_H

#if defined ARCH_32
typedef uint32_t ADDRESS;
#elif defined ARCH_64
typedef uint64_t ADDRESS;
#else
#error Please specify an architecture {ARCH_32 or ARCH_64}
#endif

#endif