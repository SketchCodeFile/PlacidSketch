#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_

// Platform-specific functions and macros
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

void MurmurHash3_x86_32(const void* key, int len, uint32_t seed, void* out);
void MurmurHash3_x86_128(const void* key, int len, uint32_t seed, void* out);
void MurmurHash3_x64_128(const void* key, const int len, const uint32_t seed, void* out);
uint64_t MurmurHash64B(const void* key, int len, unsigned int seed);
uint64_t MurmurHash32(const void* key, int len, unsigned int seed);

#endif  // _MURMURHASH3_H_
