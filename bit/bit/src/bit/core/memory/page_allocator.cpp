#include <bit/core/memory/page_allocator.h>
#include <bit/core/memory.h>
#include <bit/core/os/debug.h>

bit::PageAllocator::PageAllocator(const char* Name, void* StartAddress, size_t RegionSize, size_t AllocationGranularity) :
	IAllocator(Name),
	PageGranularity(AllocationGranularity)
{
	BIT_ASSERT(bit::VirtualReserveSpace(StartAddress, RegionSize, VirtualAddress));
	BIT_ASSERT(VirtualAddress.GetRegionSize() > 0); // Can't be 0
	BIT_ASSERT(VirtualAddress.IsValid());
	BIT_ASSERT(bit::IsPow2(VirtualAddress.GetRegionSize())); // ***  TotalSize is not a power of 2 value. TotalSize MUST be power of 2. *** 
	LevelCount = bit::BitScanReverse(VirtualAddress.GetRegionSize() / PageGranularity) + 1;
	PageCount = bit::Pow2(LevelCount) - 1; // We subtract 1 because we can't have a block at the end of memory.
	size_t PageBitCount = PageCount * BITS_PER_PAGE;
	size_t PageByteCount = PageBitCount / 8;
	// TODO: Since levels are store contigously we could dynamically allocate levels as pages.
	// The only situation when we would need to allocate all levels is when a allocation
	// of size Granularity is made.
	BIT_ASSERT(bit::VirtualReserveSpace(nullptr, PageByteCount, BitArray)); // Failed to allocate space for bit array ?
	BitArray.CommitAll();
	bit::Memset(BitArray.GetBaseAddress(), 0, PageByteCount);
#if BIT_BUILD_DEBUG
	BIT_LOG("Page Allocator Setup Usage: %.2f KiB", (float)(sizeof(bit::PageAllocator) + PageByteCount) / 1024.0f);
#endif
}

bit::PageAllocator::~PageAllocator()
{
	bit::VirtualReleaseSpace(VirtualAddress);
	bit::VirtualReleaseSpace(BitArray);
}

void* bit::PageAllocator::CommitPage(void* Address, size_t Size)
{
	return VirtualAddress.CommitPagesByAddress(Address, Size);
}

void bit::PageAllocator::DecommitPage(void* Address, size_t Size)
{
	VirtualAddress.DecommitPagesByAddress(Address, Size);
}

bool bit::PageAllocator::SetProtectionPage(void* Address, size_t Size, PageProtectionType ProtectionType)
{
	return VirtualAddress.ProtectPagesByAddress(Address, Size, ProtectionType);
}

const char* bit::PageAllocator::GetStateName(PageState State)
{
	switch (State)
	{
	case bit::PageAllocator::PAGE_STATE_FREE:
		return "BUDDY_PAGE_STATE_FREE";
	case bit::PageAllocator::PAGE_STATE_SPLIT:
		return "BUDDY_PAGE_STATE_SPLIT";
	case bit::PageAllocator::PAGE_STATE_USED:
		return "BUDDY_PAGE_STATE_USED";
	}
	return "BUDDY_PAGE_STATE_INVALID";
}

void bit::PageAllocator::DebugPrintState(size_t PageIndex, size_t Depth)
{
	if (PageIndex < PageCount)
	{
		for (size_t Index = 0; Index < Depth; ++Index)
		{
			BIT_LOG("\t");
		}
		BIT_LOG("%04u 0x%08X 0x%08X %s", PageIndex, (uintptr_t)GetPageAddress(PageIndex), GetPageSize(PageIndex), GetStateName(GetPageState(PageIndex)));
		DebugPrintState(GetPageLeftChildIdx(PageIndex), Depth + 1);
		DebugPrintState(GetPageRightChildIdx(PageIndex), Depth + 1);
	}
}

void bit::PageAllocator::SetPageState(size_t PageIndex, PageState State)
{
	BIT_ASSERT(PageIndex < PageCount);
	uint8_t BitPos = (PageIndex % 4) * 2;
	(reinterpret_cast<uint8_t*>(BitArray.GetBaseAddress())[PageIndex / 4] &= ~(3 << BitPos)) |= (((uint8_t)State & 3) << BitPos);
}

bit::PageAllocator::PageState bit::PageAllocator::GetPageState(size_t PageIndex)
{
	BIT_ASSERT(PageIndex < PageCount);
	return (PageState)((reinterpret_cast<uint8_t*>(BitArray.GetBaseAddress())[PageIndex / 4] >> ((PageIndex % 4) * 2)) & 3);
}

size_t bit::PageAllocator::GetPageLevel(size_t PageIndex)
{
	return bit::Clamp(bit::BitScanReverse(PageIndex + 1), (size_t)0, (size_t)LevelCount - 1);
}

size_t bit::PageAllocator::GetPageSize(size_t PageIndex)
{
	return bit::Clamp(VirtualAddress.GetRegionSize() >> GetPageLevel(PageIndex), PageGranularity, VirtualAddress.GetRegionSize());
}

size_t bit::PageAllocator::GetPageIndex(size_t Level, size_t Slot)
{
	return (bit::Pow2(Level) - 1) + Slot;
}

size_t bit::PageAllocator::GetPageSlot(size_t Level, size_t PageIndex)
{
	return PageIndex - (bit::Pow2(Level) - 1);
}

size_t bit::PageAllocator::GetPageLeftChildIdx(size_t PageIndex)
{
	return (2 * PageIndex) + 1;
}

size_t bit::PageAllocator::GetPageRightChildIdx(size_t PageIndex)
{
	return (2 * PageIndex) + 2;
}

size_t bit::PageAllocator::GetPageParentIdx(size_t PageIndex)
{
	return (PageIndex - 1) / 2;
}

size_t bit::PageAllocator::GetLevelForPageCountRecursive(size_t PageTotalSize, size_t CurrentLevel)
{
	size_t LevelSize = VirtualAddress.GetRegionSize() / bit::Pow2(CurrentLevel);
	if (LevelSize > PageTotalSize) return GetLevelForPageCountRecursive(PageTotalSize, CurrentLevel + 1);
	else if (LevelSize < PageTotalSize) return CurrentLevel - 1;
	else return CurrentLevel;
}

size_t bit::PageAllocator::GetLevelForSize(size_t Size)
{
	if (Size == 0 || Size > VirtualAddress.GetRegionSize()) return INVALID_LEVEL;
	return bit::BitScanReverse(VirtualAddress.GetRegionSize() / RoundToPageGranularity(Size));
}

void* bit::PageAllocator::GetPageAddress(size_t PageIndex)
{
	const size_t PageLevel = GetPageLevel(PageIndex);
	const size_t SlotIndex = GetPageSlot(PageLevel, PageIndex);
	const size_t PageSize = VirtualAddress.GetRegionSize() >> PageLevel;
	return bit::OffsetPtr(VirtualAddress.GetBaseAddress(), SlotIndex * PageSize);
}

size_t bit::PageAllocator::GetPageIndex(const void* Address)
{
	uintptr_t Ptr = (uintptr_t)Address;
	uintptr_t Start = (uintptr_t)VirtualAddress.GetBaseAddress();
	uintptr_t Diff = Ptr - Start;
	uintptr_t Slot = Diff / PageGranularity;
	size_t PageIndex = GetPageIndex(LevelCount - 1, Slot);
	size_t PrevPageIndex = PageIndex;

	if (PageIndex > PageCount - 1)
		return INVALID_PAGE;

	while (GetPageState(PageIndex) == PAGE_STATE_USED)
	{
		PrevPageIndex = PageIndex;
		PageIndex = GetPageParentIdx(PageIndex);
	}
#if _DEBUG
	BIT_ASSERT(GetPageAddress(PrevPageIndex) == Address); // Page Index address doesn't match the address input
#endif
	return PrevPageIndex;
}

void* bit::PageAllocator::Allocate(size_t Size, size_t Alignment)
{
	size_t AlignedSize = bit::AlignUint(Size, bit::Max(GetPageGranularity(), Alignment));
	ReservedPages Pages = Reserve(Size);
	if (Pages.Address != nullptr)
	{
		// To avoid over committing memory we'll just round up to the page granularity
		// We'll still waste address space, but physical memory won't be wasted.
		return CommitPage(Pages.Address, RoundToPageGranularity(Size));
	}
	BIT_PANIC(); // Out of memory ??
	return nullptr;
}

void* bit::PageAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	BIT_UNUSED_VAR(Alignment); // Alignment will be page size.
	if (Pointer != nullptr)
	{
		size_t OldSize = GetSize(Pointer);
		void* NewBlock = Allocate(Size, Alignment);
		bit::Memcpy(NewBlock, Pointer, OldSize);
		Free(Pointer);
		return NewBlock;
	}
	return Allocate(Size, Alignment);
}

void bit::PageAllocator::Free(void* Address)
{
	size_t PageIndex = GetPageIndex(Address);
	if (PageIndex != INVALID_PAGE)
	{
		FreePage(PageIndex);
		DecommitPage(Address, GetPageSize(PageIndex)); // Maybe I should batch this, specially if I coalesce free pages
	}
}

bit::PageAllocator::ReservedPages bit::PageAllocator::Reserve(size_t Size)
{
	ReservedPages Pages = {};
	size_t PageIndex = AllocPage(Size);
	if (PageIndex != INVALID_PAGE)
	{
		Pages.PageIndex = PageIndex;
		Pages.Address = GetPageAddress(PageIndex);
		Pages.ReservedSize = GetPageSize(PageIndex);
		Pages.RequestedSize = Size;
	}
	return Pages;
}

void* bit::PageAllocator::Commit(void* Address, size_t Size)
{
	return CommitPage(Address, Size);
}

void bit::PageAllocator::Decommit(void* Address, size_t Size)
{
	DecommitPage(Address, Size);
}

bool bit::PageAllocator::SetProtection(void* Address, size_t Size, PageProtectionType ProtectionType)
{
	return SetProtectionPage(Address, Size, ProtectionType);
}

size_t bit::PageAllocator::GetSize(void* Address)
{
	return GetPageSize(GetPageIndex(Address));
}

bit::MemoryUsageInfo bit::PageAllocator::GetMemoryUsageInfo()
{
	MemoryUsageInfo MemInfo = {};
	MemInfo.ReservedBytes = VirtualAddress.GetRegionSize() + BitArray.GetRegionSize();
	MemInfo.CommittedBytes = VirtualAddress.GetCommittedSize() + BitArray.GetCommittedSize();
	MemInfo.AllocatedBytes = VirtualAddress.GetCommittedSize() + BitArray.GetCommittedSize();
	return MemInfo;
}

bool bit::PageAllocator::CanAllocate(size_t Size, size_t Alignment)
{
	return Size >= PageGranularity || Alignment >= PageGranularity;
}

bool bit::PageAllocator::OwnsAllocation(const void* Ptr)
{
	return VirtualAddress.OwnsAddress(Ptr);
}

size_t bit::PageAllocator::GetPageGranularity()
{
	return PageGranularity;
}

size_t bit::PageAllocator::FindPageRecursive(size_t PageIndex, size_t MinSize)
{
	size_t PageSize = GetPageSize(PageIndex);
	PageState State = GetPageState(PageIndex);

	if (State != PAGE_STATE_USED && PageSize >= MinSize)
	{
		size_t ChildPageSize = GetPageSize(GetPageLeftChildIdx(PageIndex));
		if (State == PAGE_STATE_FREE && (ChildPageSize < MinSize || PageSize == MinSize))
		{
			SetPageState(PageIndex, PAGE_STATE_USED);
			MarkChildPagesRecursive(PageIndex, PAGE_STATE_USED);
			MarkParentPagesRecursive(PageIndex, PAGE_STATE_SPLIT);
			return PageIndex;
		}
		else
		{
			size_t LeftPage = FindPageRecursive(GetPageLeftChildIdx(PageIndex), MinSize);
			if (LeftPage != INVALID_PAGE && GetPageState(LeftPage) == PAGE_STATE_USED)
			{
				return LeftPage;
			}

			size_t RightPage = FindPageRecursive(GetPageRightChildIdx(PageIndex), MinSize);
			if (RightPage != INVALID_PAGE && GetPageState(RightPage) == PAGE_STATE_USED)
			{
				return RightPage;
			}
		}
	}

	return INVALID_PAGE;
}

size_t bit::PageAllocator::AllocPage(size_t Size)
{
	size_t PageSize = RoundToPageGranularity(Size);
	size_t Level = GetLevelForSize(Size);
	size_t PageIndex = GetPageIndex(Level, 0);
	size_t PageCount = VirtualAddress.GetRegionSize() / PageSize;
	for (size_t Index = 0; Index < PageCount; ++Index)
	{
		if (GetPageState(PageIndex + Index) == PAGE_STATE_FREE)
		{
			PageIndex += Index;
			SetPageState(PageIndex, PAGE_STATE_USED);
			MarkChildPagesRecursive(PageIndex, PAGE_STATE_USED);
			MarkParentPagesRecursive(PageIndex, PAGE_STATE_SPLIT);
			return PageIndex;
		}
	}
#if 0
	BIT_LOG("*** Allocation Failed ***---- BEGIN ALLOCATOR STATE ----");
	DebugPrintState();
	BIT_LOG("---- END ALLOCATOR STATE ----");
#endif
	return INVALID_PAGE;
}

void bit::PageAllocator::CoalescePagesRecursive(size_t PageIndex)
{
	size_t ParentPage = GetPageParentIdx(PageIndex);
	if (ParentPage < PageCount - 1 && GetPageState(ParentPage) == PAGE_STATE_SPLIT)
	{
		size_t LeftChild = GetPageLeftChildIdx(ParentPage);
		size_t RightChild = GetPageRightChildIdx(ParentPage);
		if (GetPageState(LeftChild) == PAGE_STATE_FREE && GetPageState(RightChild) == PAGE_STATE_FREE)
		{
			SetPageState(ParentPage, PAGE_STATE_FREE);
			CoalescePagesRecursive(ParentPage);
		}
	}
}

void bit::PageAllocator::FreePage(size_t PageIndex)
{
	SetPageState(PageIndex, PAGE_STATE_FREE);
	MarkChildPagesRecursive(PageIndex, PAGE_STATE_FREE);
	CoalescePagesRecursive(PageIndex);
}

void bit::PageAllocator::MarkChildPagesRecursive(size_t PageIndex, PageState State)
{
	size_t LeftChildIndex = GetPageLeftChildIdx(PageIndex);
	size_t RightChildIndex = GetPageRightChildIdx(PageIndex);
	size_t PageLevel = GetPageLevel(PageIndex);

	if (PageLevel < (LevelCount - 1))
	{
		SetPageState(LeftChildIndex, State);
		SetPageState(RightChildIndex, State);
		MarkChildPagesRecursive(GetPageLeftChildIdx(PageIndex), State);
		MarkChildPagesRecursive(GetPageRightChildIdx(PageIndex), State);
	}
}

void bit::PageAllocator::MarkParentPagesRecursive(size_t PageIndex, PageState State)
{
	size_t ParentIndex = GetPageParentIdx(PageIndex);
	SetPageState(ParentIndex, State);
	if (GetPageLevel(ParentIndex) > 0)
	{
		MarkParentPagesRecursive(ParentIndex, State);
	}
}

size_t bit::PageAllocator::RoundToPageGranularity(size_t Size)
{
	if (Size <= PageGranularity) return PageGranularity;
	size_t NewSize = (Size / PageGranularity + 1) * PageGranularity;
	return NewSize;
}

