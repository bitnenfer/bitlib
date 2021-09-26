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
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		MemoryUsageInfo GetMemoryUsageInfo() override;

	private:
		MemoryArena Arena;
		size_t BufferOffset;
	};
}