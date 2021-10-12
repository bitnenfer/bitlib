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
			FreePageLink* NextPage;
		};

		struct PageMetadata
		{
			int64_t PageIndex;
			int64_t AllocatedBytes;
			size_t AssignedSize;
			PageMetadata* NextFreePage;
		};

		struct FreeBlockLink
		{
			FreeBlockLink* PrevBlock;
			FreeBlockLink* NextBlock;
		};

		struct BlockMetadata
		{
			int64_t AllocatedBytes;
			FreeBlockLink* FreeList;
		};

		static constexpr uint32_t SMALL_SIZE_ALLOCATOR_MAGIC = 0xDEADBEEF;
		static constexpr size_t ADDRESS_SPACE_SIZE = 512 * 1024 * 1024;
		static constexpr size_t PAGE_SIZE = 64 * 1024;
		static constexpr size_t MIN_DECOMMIT_SIZE = 2 * 1024 * 1024;
		static constexpr size_t NUM_OF_PAGES = ADDRESS_SPACE_SIZE / PAGE_SIZE;
		static constexpr size_t SIZE_OF_BOOKKEEPING = sizeof(PageMetadata) * NUM_OF_PAGES;
		static constexpr size_t TOTAL_ADDRESS_SPACE_SIZE = ADDRESS_SPACE_SIZE + SIZE_OF_BOOKKEEPING;
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
		size_t GetBlockSize(size_t BlockIndex);
		size_t GetBlockIndex(size_t BlockSize);
		FreeBlockLink* GetFreeBlock(size_t BlockIndex);
		FreeBlockLink* GetFreeBlockFromNewPage(size_t BlockIndex);
		FreePageLink* AllocateNewPage();
		FreePageLink* GetFreePage();
		void PushBlockToFreeList(size_t BlockIndex, FreeBlockLink* FreeBlock);
		FreeBlockLink* UnlinkFreeBlock(size_t BlockIndex, FreeBlockLink* FreeBlock);
		PageMetadata* GetPageData(const void* Ptr);
		void* GetPageBaseByAddress(const void* Ptr);
		void* GetPageBaseByAddressByIndex(size_t PageIndex);
		size_t GetPageDataIndex(const void* Ptr);
		void OnAlloc(size_t BlockIndex, size_t PageIndex);
		void OnFree(size_t BlockIndex, size_t PageIndex);
		void FreePage(size_t PageIndex);
		size_t DecommitFreePages();

	private:
		BlockMetadata Blocks[NUM_OF_SIZES];
		PageMetadata Pages[NUM_OF_PAGES];
		VirtualAddressSpace Memory;
		PageMetadata* PageDecommitList;
		FreePageLink* PageFreeList;
		void* BaseVirtualAddress;
		int64_t BaseVirtualAddressOffset;
		int64_t PageFreeListBytes;
		int64_t AllocatedBytes;
		int64_t CommittedBytes;
	};
}