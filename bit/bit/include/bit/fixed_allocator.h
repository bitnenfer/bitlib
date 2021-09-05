#pragma once

#include <bit/allocator.h>

namespace bit
{
	template<size_t CapacityInBytes>
	struct TFixedMemoryArena : public CMemoryArena
	{
		TFixedMemoryArena() :
			CMemoryArena(&Data[0], CapacityInBytes)
		{}

		uint8_t Data[CapacityInBytes];
	};
}
