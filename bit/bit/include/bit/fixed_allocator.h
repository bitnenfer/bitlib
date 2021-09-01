#pragma once

#include <bit/allocator.h>

namespace bit
{
	template<size_t Capacity>
	struct TFixedAllocator : public IAllocator
	{
		TFixedAllocator(const char* Name = "TFixedAllocator") :
			IAllocator::IAllocator(Name)
		{
			bit::Memset(Data, 0, sizeof(Data));
		}

		void* Allocate(size_t Size, size_t Alignment) override
		{
			return (void*)Data;
		}
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override
		{
			return (void*)Data;
		}
		void Free(void* Pointer) override
		{
		}
		size_t GetSize(void* Pointer) override
		{
			return Capacity;
		}
		CMemoryInfo GetMemoryInfo() override
		{
			CMemoryInfo Info = {};
			Info.AllocatedBytes = Capacity;
			Info.CommittedBytes = Capacity;
			Info.ReservedBytes = Capacity;
			return Info;
		}

		uint8_t Data[Capacity];
	};
}
