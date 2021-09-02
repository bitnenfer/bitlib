#pragma once

#include <bit/types.h>
#include <bit/memory.h>

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
