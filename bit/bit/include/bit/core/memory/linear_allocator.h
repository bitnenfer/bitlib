#pragma once

#include <bit/core/memory/allocator.h>

namespace bit
{
	struct BITLIB_API LinearAllocator : public IAllocator
	{
		static size_t GetRequiredAlignment();

		LinearAllocator(const char* Name, const MemoryArena& Arena);
		~LinearAllocator();

		void Reset();
		void* Allocate(size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		AllocatorMemoryInfo GetMemoryUsageInfo() override;
		bool CanAllocate(size_t Size, size_t Alignment) override;
		bool OwnsAllocation(const void* Ptr) override;

	private:
		MemoryArena Arena;
		size_t BufferOffset;
	};
}