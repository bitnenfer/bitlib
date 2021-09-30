#pragma once

#include <bit/core/types.h>
#include <bit/core/memory/small_size_allocator.h>
#include <bit/core/memory/medium_size_allocator.h>
#include <bit/core/os/mutex.h>

namespace bit
{
	struct BITLIB_API MemoryManager : public IAllocator
	{
		MemoryManager();
		virtual void* Allocate(size_t Size, size_t Alignment) override;
		virtual void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		virtual void Free(void* Pointer) override;
		virtual size_t GetSize(void* Pointer) override;
		virtual AllocatorMemoryInfo GetMemoryUsageInfo() override;
		bool CanAllocate(size_t Size, size_t Alignment) override;
		bool OwnsAllocation(const void* Ptr) override;
		size_t Compact() override;

	private:
		SmallSizeAllocator SmallAllocator;
		MediumSizeAllocator MediumAllocator;
		Mutex AccessLock;
	};
}
