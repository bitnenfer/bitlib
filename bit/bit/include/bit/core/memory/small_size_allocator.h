#pragma once

#include <bit/core/memory/allocator.h>
#include <bit/core/os/virtual_memory.h>

#define BIT_SMALL_SIZE_ALLOCATOR_USE_LARGE_BLOCKS 1

namespace bit
{
	struct BITLIB_API SmallSizeAllocator
	{
		struct BITLIB_API FreePage
		{
			FreePage* Next;
		};

		struct BITLIB_API FreeBlock
		{
		#if BIT_SMALL_SIZE_ALLOCATOR_USE_LARGE_BLOCKS
			FreeBlock* Prev;
		#endif
			FreeBlock* Next;
		};

		struct BITLIB_API PageAllocationInfo
		{
			int64_t AllocatedBytes;
			int16_t AssignedSize;
			bool bCommitted;
		};

		struct BITLIB_API BlockInfo
		{
			int64_t AllocatedBytes;
			FreeBlock* FreeList;
		};

		static constexpr size_t USABLE_ADDRESS_SPACE_SIZE = 512 MiB;
		static constexpr size_t PAGE_SIZE = 16 KiB;
		static constexpr size_t NUM_OF_PAGES = USABLE_ADDRESS_SPACE_SIZE / PAGE_SIZE;
		static constexpr size_t SIZE_OF_BOOKKEEPING = sizeof(PageAllocationInfo) * NUM_OF_PAGES;
		static constexpr size_t TOTAL_ADDRESS_SPACE_SIZE = USABLE_ADDRESS_SPACE_SIZE + SIZE_OF_BOOKKEEPING;
		static constexpr size_t MIN_ALLOCATION_SIZE = sizeof(FreeBlock);
		static constexpr size_t MAX_ALLOCATION_SIZE = 512;
		static constexpr size_t NUM_OF_SIZES = MAX_ALLOCATION_SIZE / MIN_ALLOCATION_SIZE;

		static_assert((MAX_ALLOCATION_SIZE % MIN_ALLOCATION_SIZE) == 0, "MAX_ALLOCATION_SIZE must be divisible by MIN_ALLOCATION_SIZE");

		SmallSizeAllocator();
		void* Allocate(size_t Size, size_t Alignment);
		void Free(void* Pointer);
		size_t GetSize(void* Pointer);
		AllocatorMemoryInfo GetMemoryUsageInfo();
		bool CanAllocate(size_t Size, size_t Alignment);
		bool OwnsAllocation(const void* Ptr);
		size_t Compact();

	private:
		size_t GetBlockSize(size_t BlockIndex);
		size_t SelectBlockIndex(size_t Size);
		void* GetPageBaseByAddress(const void* Address);
		size_t GetPageAllocationInfoIndex(const void* Address);
		void TrackAllocation(size_t BlockIndex, size_t PageIndex, int64_t Size);
		void* AllocateFreeBlock(size_t BlockSize, size_t BlockIndex);
		void* GetPageAddress(size_t PageIndex);
		void* AllocateNewPage(size_t BlockSize);
		void* AllocateFromNewPage(size_t BlockSize, size_t BlockIndex);
		void UnlinkFreeBlock(FreeBlock* Block, BlockInfo& Info);
		void* PopFreeBlock(size_t BlockIndex);
		void PushFreeBlockToFreeList(FreeBlock* Block, size_t BlockIndex);
		void PushPointerToFreeList(void* Pointer, size_t Size);
		void PushPageToFreeBlocks(void* Pages, size_t BlockSize, size_t BlockIndex);

	private:
		BlockInfo Blocks[NUM_OF_SIZES];
		VirtualAddressSpace Memory;
		void* BaseVirtualAddress;
		size_t BaseVirtualAddressOffset;
		FreePage* PageFreeList;
		PageAllocationInfo* PageInfo;
		int64_t AllocatedBytes;
	};
}
