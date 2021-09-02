#pragma once

#include <bit/types.h>
#include <bit/memory.h>
#include <bit/allocator.h>

#define BIT_USE_INLINE_DATA 1
#define BIT_INLINE_DATA_COUNT 32

namespace bit
{
	struct CDefaultAllocator
	{
		template<typename T>
		T* Allocate(T* Original, size_t Count)
		{
			if (Original != nullptr)
			{
				return (T*)bit::Realloc(Original, Count * sizeof(T), alignof(T));
			}
			return (T*)bit::Malloc(Count * sizeof(T), alignof(T));
		}

		template<typename T>
		void Free(T* Ptr)
		{
			bit::Free(Ptr);
		}
	};
}
