#pragma once

#include <bit/allocator.h>

namespace bit
{
	template<typename T, size_t Capacity>
	struct FixedAllocator : public IAllocator
	{
		virtual void* Alloc(size_t Size, size_t Alignment) override
		{
			return Data;
		}
		virtual void* Realloc(void* Pointer, size_t Size, size_t Alignment) override
		{
			return Data;
		}
		virtual void Free(void* Pointer) override
		{
		}
		virtual size_t GetSize(void* Pointer) override
		{
			return Capacity * sizeof(T);
		}
		virtual size_t GetTotalUsedMemory() override
		{
			return Capacity * sizeof(T);
		}
		virtual const char* GetName() override
		{
			return "FixedAllocator";
		}

		T Data[Capacity];
	};
}
