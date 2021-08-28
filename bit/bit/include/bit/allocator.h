#pragma once

#include <bit/types.h>

namespace bit
{
	static constexpr size_t DEAFULT_ALIGNMENT = 4;

	struct BIT_API IAllocator
	{
		virtual void* Alloc(size_t Size, size_t Alignment) = 0;
		virtual void* Realloc(void* Pointer, size_t Size, size_t Alignment) = 0;
		virtual void Free(void* Pointer) = 0;
		virtual size_t GetSize(void* Pointer) = 0;
		virtual size_t GetTotalUsedMemory() = 0;
		virtual const char* GetName() = 0;
	};
}
