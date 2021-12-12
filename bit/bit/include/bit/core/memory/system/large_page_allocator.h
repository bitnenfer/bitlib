#pragma once

#include <bit/core/os/virtual_memory.h>
#include <bit/core/memory/allocator.h>
#include <bit/container/hash_table.h>

namespace bit
{
	struct LargePageAllocator : public IAllocator
	{
		struct AllocationEntry
		{
			uint64_t AllocatedBytes;
			uint64_t CommittedBytes;
		};
		LargePageAllocator();
		virtual ~LargePageAllocator();
		virtual void* Allocate(size_t Size, size_t Alignment) override;
		virtual void Free(void* Pointer) override;
		virtual size_t GetSize(void* Pointer) override;
		virtual AllocatorMemoryInfo GetMemoryUsageInfo() override;
		virtual bool CanAllocate(size_t Size, size_t Alignment) override;
		virtual bool OwnsAllocation(const void* Ptr) override;
		
	private:
		bit::HashTable<void*, AllocationEntry> AllocationMap;
		bit::VirtualMemoryBlock AllocationMapBackstore;
	};
}
