#pragma once

#include <bit/types.h>

namespace bit
{
	#if BIT_PLATFORM_X64
	static constexpr size_t DEFAULT_HASH_SEED = 0xDEADBEEFDEADBEEF;
	#elif BIT_PLATFORM_X86
	static constexpr size_t DEFAULT_HASH_SEED = 0xDEADBEEF;
	#endif
	size_t MurmurHash(const void* Key, size_t Len, size_t Seed);
}
