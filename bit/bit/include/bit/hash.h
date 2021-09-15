#pragma once

#include <bit/murmur_hash.h>

namespace bit
{
	template<typename T>
	struct THash
	{
		typedef size_t HashType_t;
		HashType_t operator()(const T& Value) const
		{
			return bit::MurmurHash(&Value, sizeof(T), bit::DEFAULT_HASH_SEED);
		}
	};
}
