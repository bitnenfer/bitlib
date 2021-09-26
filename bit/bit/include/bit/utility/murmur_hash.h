#pragma once

#include <bit/core/types.h>

namespace bit
{
	#if BIT_PLATFORM_X64
	static constexpr size_t DEFAULT_HASH_SEED = 0xDEADBEEFDEADBEEF;
	#elif BIT_PLATFORM_X86
	static constexpr size_t DEFAULT_HASH_SEED = 0xDEADBEEF;
	#endif
	BITLIB_API size_t MurmurHash(const void* Key, size_t Len, size_t Seed);

	template<typename T>
	struct MurmurHasher
	{
		size_t operator()(const T& Value) const
		{
			return bit::MurmurHash(&Value, sizeof(T), bit::DEFAULT_HASH_SEED);
		}
	};
}
