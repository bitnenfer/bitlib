#pragma once

#include <bit/virtual_memory.h>

namespace bit
{

	/* Uses a Binary Buddy Allocator algorithm */
	/* Alignment isn't respected here. It'll be aligned to page size */
	struct BITLIB_API CPageAllocator : public bit::IAllocator
	{
		static constexpr size_t DEFAULT_PAGE_GRANULARITY = 1 << 16; // 64 KiB Pages
		static constexpr size_t BITS_PER_PAGE = 2; // 2 bits because we want to have 3 states (free = 00, split, 01, used = 10)
		static constexpr size_t INVALID_LEVEL = 0xFFFFFFFF;
		static constexpr size_t INVALID_PAGE = 0xFFFFFFFF;
		static constexpr size_t DEFAULT_PAGE_ALIGNMENT = 0; // We don't use alignment here. By default it'll be aligned to page size.

		CPageAllocator(const char* Name, void* StartAddress, size_t RegionSize, size_t AllocationGranularity = DEFAULT_PAGE_GRANULARITY);
		~CPageAllocator();

		void* Allocate(size_t Size, size_t Alignment = DEFAULT_PAGE_ALIGNMENT) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment = DEFAULT_PAGE_ALIGNMENT) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		CMemoryUsageInfo GetMemoryUsageInfo() override;

	protected:
		void DebugPrintState(size_t PageIndex = 0, size_t Depth = 0);

		// These functions won't affect the tracked memory. 
		// This just determines if the virtual address is mapped to physical pages
		// Useful for more granular control over allocated pages.
		struct CReservePages
		{
			void* Address;
			size_t RequestedSize;
			size_t ReservedSize;
			size_t PageIndex;
		};
		CReservePages Reserve(size_t Size);
		void* Commit(void* Address, size_t Size);
		void Decommit(void* Address, size_t Size);
		bool SetProtection(void* Address, size_t Size, EPageProtectionType ProtectionType);
		size_t GetPageGranularity();

	private:
		enum EPageState
		{
			PAGE_STATE_FREE = 0b00,
			PAGE_STATE_SPLIT = 0b01,
			PAGE_STATE_USED = 0b10
		};

		void* CommitPage(void* Address, size_t Size);
		void DecommitPage(void* Address, size_t Size);
		bool SetProtectionPage(void* Address, size_t Size, EPageProtectionType ProtectionType);
		const char* GetStateName(EPageState State);
		void SetPageState(size_t PageIndex, EPageState State);
		EPageState GetPageState(size_t PageIndex);
		size_t GetPageLevel(size_t PageIndex);
		size_t GetPageSize(size_t PageIndex);
		size_t GetPageIndex(size_t Level, size_t Slot);
		size_t GetPageSlot(size_t Level, size_t PageIndex);
		size_t GetPageLeftChildIdx(size_t PageIndex);
		size_t GetPageRightChildIdx(size_t PageIndex);
		size_t GetPageParentIdx(size_t PageIndex);
		size_t GetLevelForPageCountRecursive(size_t PageTotalSize, size_t CurrentLevel);
		size_t GetLevelForSize(size_t Size);
		void* GetPageAddress(size_t PageIndex);
		size_t GetPageIndex(const void* pAddress);
		size_t FindPageRecursive(size_t PageIndex, size_t MinSize);
		size_t AllocPage(size_t Size);
		void CoalescePagesRecursive(size_t PageIndex);
		void FreePage(size_t PageIndex);
		void MarkChildPagesRecursive(size_t PageIndex, EPageState State);
		void MarkParentPagesRecursive(size_t PageIndex, EPageState State);
		size_t RoundToPageGranularity(size_t Size);

	private:
		CVirtualAddressSpace VirtualAddressSpace;
		uint8_t* BitArray;
		size_t PageGranularity;
		size_t PageCount;
		size_t LevelCount;
	};
}
