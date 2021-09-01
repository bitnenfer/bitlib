#pragma once

#include <bit/types.h>
#include <bit/memory.h>

namespace bit
{
	template<typename T, size_t TAlignment = sizeof(T)>
	struct TDefaultAllocator
	{
		T* Allocate(T* Original, size_t Count)
		{
			if (Original != nullptr)
			{
				return (T*)bit::Realloc(Original, Count * sizeof(T), TAlignment);
			}
			return (T*)bit::Malloc(Count * sizeof(T), TAlignment);
		}

		void Free(T* Ptr)
		{
			bit::Free(Ptr);
		}
	};
}
