#pragma once

#include <bit/core/types.h>
#include <bit/core/os/virtual_memory.h>
#include <bit/core/os/debug.h>
#include <bit/core/memory.h>

#define SMALL_SIZE_ALLOCATOR_MARK_BLOCKS 1

namespace bit
{
	struct BITLIB_API SmallSizeAllocator
	{
		struct FreePageLink
		{
			FreePageLink* Next;
		};

		struct FreeBlockLink
		{
			FreeBlockLink* Next;
		};

		struct PageLink
		{
			int64_t AllocatedBytes;
			int64_t AssignedSize;
			PageLink* Prev;
			PageLink* Next;
			FreeBlockLink* FreeList;
		};

		struct BlockData
		{
			int64_t AllocatedBytes;
			PageLink* Pages;
		};

		static constexpr size_t USABLE_ADDRESS_SPACE_SIZE = 512 * 1024 * 1024;
		static constexpr size_t PAGE_SIZE = 64 * 1024;
		static constexpr size_t MIN_DECOMMIT_SIZE = PAGE_SIZE;
		static constexpr size_t NUM_OF_PAGES = USABLE_ADDRESS_SPACE_SIZE / PAGE_SIZE;
		static constexpr size_t SIZE_OF_BOOKKEEPING = sizeof(PageLink) * NUM_OF_PAGES;
		static constexpr size_t TOTAL_ADDRESS_SPACE_SIZE = USABLE_ADDRESS_SPACE_SIZE + SIZE_OF_BOOKKEEPING;
		static constexpr size_t MIN_ALLOCATION_SIZE = sizeof(FreeBlockLink);
		static constexpr size_t MAX_ALLOCATION_SIZE = 32 * 1024;
		static constexpr size_t NUM_OF_SIZES = MAX_ALLOCATION_SIZE / MIN_ALLOCATION_SIZE;
		static_assert((MAX_ALLOCATION_SIZE% MIN_ALLOCATION_SIZE) == 0, "MAX_ALLOCATION_SIZE must be divisible by MIN_ALLOCATION_SIZE");

		SmallSizeAllocator();
		void* Allocate(size_t Size, size_t Alignment);
		void Free(void* Pointer);
		size_t GetSize(void* Pointer);
		AllocatorMemoryInfo GetMemoryUsageInfo();
		bool CanAllocate(size_t Size, size_t Alignment);
		bool OwnsAllocation(const void* Ptr);
		size_t Compact();

	private:
		void PushPageToDecommitFreeList(PageLink* Info);
		PageLink* PopPageFromDecommitFreeList();
		size_t GetBlockSize(size_t BlockIndex);
		size_t SelectBlockIndex(size_t Size);
		void* GetPageBaseByAddress(const void* Address);
		size_t GetPageAllocationInfoIndex(const void* Address);
		PageLink* GetPageAllocationInfo(const void* Address);
		void* GetPageFromAllocationInfo(PageLink* PageInfoPtr);
		void UnlinkPage(size_t BlockIndex, size_t PageIndex);
		void PushPageToFreeList(size_t PageIndex);
		void RecordAllocation(size_t BlockIndex, size_t PageIndex, int64_t Size);
		void* AllocateFreeBlock(size_t BlockSize, size_t BlockIndex);
		void* GetPageAddress(size_t PageIndex);
		void* AllocateNewPage(size_t BlockSize);
		void SwapPages(PageLink* A, PageLink* B);
		void* PopFreeBlock(size_t BlockIndex);
		void PushFreeBlockToPageFreeList(PageLink* PageInfo, FreeBlockLink* Block, size_t BlockIndex);
		void PushPointerToPageFreeList(PageLink* PageInfo, void* Pointer, size_t Size);
		void PushPageToBlockPageList(PageLink* PageInfo, size_t BlockIndex);
		void PopPageFromBlockPageList(PageLink* PageInfo, size_t BlockIndex);
		void SetPageFreeList(void* Pages, size_t BlockSize, size_t BlockIndex);
		void* AllocateFromNewPage(size_t BlockSize, size_t BlockIndex);

	private:
		BlockData Blocks[NUM_OF_SIZES];
		bit::VirtualAddressSpace Memory;
		void* BaseVirtualAddress;
		size_t BaseVirtualAddressOffset;
		FreePageLink* PageFreeList;
		PageLink* PageList;
		PageLink* DecommittedFreeList;
		int64_t AllocatedBytes;
		int64_t CommittedBytes;
		int64_t FreePageSize;
	};
}