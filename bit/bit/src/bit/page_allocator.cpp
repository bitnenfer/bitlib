#include <bit/page_allocator.h>
#include <bit/memory.h>
#include <bit/os.h>

bit::CPageAllocator::CPageAllocator(const char* Name, CVirtualMemory::CMemoryRegion InVirtualMemoryRegion, size_t AllocationGranularity) :
	IAllocator(Name),
	VirtualMemoryRegion(InVirtualMemoryRegion),
	BitArray(nullptr),
	PageGranularity(AllocationGranularity)
{
	BIT_ASSERT(VirtualMemoryRegion.GetRegionSize() > 0); // Can't be 0
	BIT_ASSERT(VirtualMemoryRegion.IsValid());
	BIT_ASSERT(bit::IsPow2(VirtualMemoryRegion.GetRegionSize())); // ***  TotalSize is not a power of 2 value. TotalSize MUST be power of 2. *** 
	LevelCount = bit::Log2(VirtualMemoryRegion.GetRegionSize() / PageGranularity) + 1;
	PageCount = bit::Pow2(LevelCount) - 1; // We subtract 1 because we can't have a block at the end of memory.
	size_t PageBitCount = PageCount * BITS_PER_PAGE;
	size_t PageByteCount = PageBitCount / 8;
	// TODO: Since levels are store contigously we could dynamically allocate levels as pages.
	// The only situation when we would need to allocate all levels is when a allocation
	// of size Granularity is made.
	BitArray = (uint8_t*)bit::Malloc(PageByteCount);
	bit::Memset(BitArray, 0, PageByteCount);
#if BIT_BUILD_DEBUG
	BIT_LOG("Page Allocator Setup Usage: %.2f KiB\n", (float)(sizeof(bit::CPageAllocator) + PageByteCount) / 1024.0f);
#endif
}

bit::CPageAllocator::~CPageAllocator()
{
	bit::CVirtualMemory::ReleaseRegion(VirtualMemoryRegion);
	bit::Free(BitArray);
}

void* bit::CPageAllocator::CommitPage(void* Address, size_t Size)
{
	return VirtualMemoryRegion.CommitPagesByAddress(Address, Size);
}

void bit::CPageAllocator::DecommitPage(void* Address, size_t Size)
{
	VirtualMemoryRegion.DecommitPagesByAddress(Address, Size);
}

bool bit::CPageAllocator::SetProtectionPage(void* Address, size_t Size, EPageProtectionType ProtectionType)
{
	return VirtualMemoryRegion.ProtectPagesByAddress(Address, Size, ProtectionType);
}

const char* bit::CPageAllocator::GetStateName(EPageState State)
{
	switch (State)
	{
	case bit::CPageAllocator::PAGE_STATE_FREE:
		return "BUDDY_PAGE_STATE_FREE";
	case bit::CPageAllocator::PAGE_STATE_SPLIT:
		return "BUDDY_PAGE_STATE_SPLIT";
	case bit::CPageAllocator::PAGE_STATE_USED:
		return "BUDDY_PAGE_STATE_USED";
	}
	return "BUDDY_PAGE_STATE_INVALID";
}

void bit::CPageAllocator::DebugPrintState(size_t PageIndex, size_t Depth)
{
	if (PageIndex < PageCount)
	{
		for (size_t Index = 0; Index < Depth; ++Index)
		{
			BIT_LOG("\t");
		}
		BIT_LOG("%04u 0x%08X 0x%08X %s\n", PageIndex, (uintptr_t)GetPageAddress(PageIndex), GetPageSize(PageIndex), GetStateName(GetPageState(PageIndex)));
		DebugPrintState(GetPageLeftChildIdx(PageIndex), Depth + 1);
		DebugPrintState(GetPageRightChildIdx(PageIndex), Depth + 1);
	}
}

void bit::CPageAllocator::SetPageState(size_t PageIndex, EPageState State)
{
	BIT_ASSERT(PageIndex < PageCount);
	uint8_t BitPos = (PageIndex % 4) * 2;
	(BitArray[PageIndex / 4] &= ~(3 << BitPos)) |= (((uint8_t)State & 3) << BitPos);
}

bit::CPageAllocator::EPageState bit::CPageAllocator::GetPageState(size_t PageIndex)
{
	BIT_ASSERT(PageIndex < PageCount);
	return (EPageState)((BitArray[PageIndex / 4] >> ((PageIndex % 4) * 2)) & 3);
}

size_t bit::CPageAllocator::GetPageLevel(size_t PageIndex)
{
	return bit::Clamp(bit::Log2(PageIndex + 1), (size_t)0, (size_t)LevelCount - 1);
}

size_t bit::CPageAllocator::GetPageSize(size_t PageIndex)
{
	return bit::Clamp(VirtualMemoryRegion.GetRegionSize() >> GetPageLevel(PageIndex), PageGranularity, VirtualMemoryRegion.GetRegionSize());
}

size_t bit::CPageAllocator::GetPageIndex(size_t Level, size_t Slot)
{
	return (bit::Pow2(Level) - 1) + Slot;
}

size_t bit::CPageAllocator::GetPageSlot(size_t Level, size_t PageIndex)
{
	return PageIndex - (bit::Pow2(Level) - 1);
}

size_t bit::CPageAllocator::GetPageLeftChildIdx(size_t PageIndex)
{
	return (2 * PageIndex) + 1;
}

size_t bit::CPageAllocator::GetPageRightChildIdx(size_t PageIndex)
{
	return (2 * PageIndex) + 2;
}

size_t bit::CPageAllocator::GetPageParentIdx(size_t PageIndex)
{
	return (PageIndex - 1) / 2;
}

size_t bit::CPageAllocator::GetLevelForPageCountRecursive(size_t PageTotalSize, size_t CurrentLevel)
{
	size_t LevelSize = VirtualMemoryRegion.GetRegionSize() / bit::Pow2(CurrentLevel);
	if (LevelSize > PageTotalSize) return GetLevelForPageCountRecursive(PageTotalSize, CurrentLevel + 1);
	else if (LevelSize < PageTotalSize) return CurrentLevel - 1;
	else return CurrentLevel;
}

size_t bit::CPageAllocator::GetLevelForSize(size_t Size)
{
	if (Size == 0 || Size > VirtualMemoryRegion.GetRegionSize()) return INVALID_LEVEL;
	return bit::Log2(VirtualMemoryRegion.GetRegionSize() / RoundToPageGranularity(Size));
}

void* bit::CPageAllocator::GetPageAddress(size_t PageIndex)
{
	const size_t PageLevel = GetPageLevel(PageIndex);
	const size_t SlotIndex = GetPageSlot(PageLevel, PageIndex);
	const size_t PageSize = VirtualMemoryRegion.GetRegionSize() >> PageLevel;
	return bit::ForwardPtr(VirtualMemoryRegion.GetBaseAddress(), SlotIndex * PageSize);
}

size_t bit::CPageAllocator::GetPageIndex(const void* Address)
{
	uintptr_t Ptr = (uintptr_t)Address;
	uintptr_t Start = (uintptr_t)VirtualMemoryRegion.GetBaseAddress();
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

void* bit::CPageAllocator::Allocate(size_t Size, size_t Alignment)
{
	BIT_UNUSED_VAR(Alignment); // Alignment will be page size.
	CReservePages Pages = Reserve(Size);
	if (Pages.Address != nullptr)
	{
		// To avoid over committing memory we'll just round up to the page granularity
		// We'll still waste address space, but physical memory won't be wasted.
		return CommitPage(Pages.Address, RoundToPageGranularity(Size));
	}
	BIT_PANIC("Out of memory"); // Out of memory ??
	return nullptr;
}

void* bit::CPageAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
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

void bit::CPageAllocator::Free(void* Address)
{
	size_t PageIndex = GetPageIndex(Address);
	if (PageIndex != INVALID_PAGE)
	{
		FreePage(PageIndex);
		DecommitPage(Address, GetPageSize(PageIndex)); // Maybe I should batch this, specially if I coalesce free pages
	}
}

bit::CPageAllocator::CReservePages bit::CPageAllocator::Reserve(size_t Size)
{
	CReservePages Pages = {};
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

void* bit::CPageAllocator::Commit(void* Address, size_t Size)
{
	return CommitPage(Address, Size);
}

void bit::CPageAllocator::Decommit(void* Address, size_t Size)
{
	DecommitPage(Address, Size);
}

bool bit::CPageAllocator::SetProtection(void* Address, size_t Size, EPageProtectionType ProtectionType)
{
	return SetProtectionPage(Address, Size, ProtectionType);
}

size_t bit::CPageAllocator::GetSize(void* Address)
{
	return GetPageSize(GetPageIndex(Address));
}

bit::CMemoryInfo bit::CPageAllocator::GetMemoryInfo()
{
	CMemoryInfo MemInfo = {};
	MemInfo.ReservedBytes = VirtualMemoryRegion.GetRegionSize();
	MemInfo.CommittedBytes = VirtualMemoryRegion.GetCommitedSize();
	MemInfo.AllocatedBytes = VirtualMemoryRegion.GetCommitedSize();
	return MemInfo;
}

size_t bit::CPageAllocator::GetPageGranularity()
{
	return PageGranularity;
}

size_t bit::CPageAllocator::FindPageRecursive(size_t PageIndex, size_t MinSize)
{
	size_t PageSize = GetPageSize(PageIndex);
	EPageState State = GetPageState(PageIndex);

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

size_t bit::CPageAllocator::AllocPage(size_t Size)
{
	size_t PageSize = RoundToPageGranularity(Size);
	size_t Level = GetLevelForSize(Size);
	size_t PageIndex = GetPageIndex(Level, 0);
	size_t PageCount = VirtualMemoryRegion.GetRegionSize() / PageSize;
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
	BIT_LOG("*** Allocation Failed ***\n---- BEGIN ALLOCATOR STATE ----\n\n");
	DebugPrintState();
	BIT_LOG("\n---- END ALLOCATOR STATE ----\n");
#endif
	return INVALID_PAGE;
}

void bit::CPageAllocator::CoalescePagesRecursive(size_t PageIndex)
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

void bit::CPageAllocator::FreePage(size_t PageIndex)
{
	SetPageState(PageIndex, PAGE_STATE_FREE);
	MarkChildPagesRecursive(PageIndex, PAGE_STATE_FREE);
	CoalescePagesRecursive(PageIndex);
}

void bit::CPageAllocator::MarkChildPagesRecursive(size_t PageIndex, EPageState State)
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

void bit::CPageAllocator::MarkParentPagesRecursive(size_t PageIndex, EPageState State)
{
	size_t ParentIndex = GetPageParentIdx(PageIndex);
	SetPageState(ParentIndex, State);
	if (GetPageLevel(ParentIndex) > 0)
	{
		MarkParentPagesRecursive(ParentIndex, State);
	}
}

size_t bit::CPageAllocator::RoundToPageGranularity(size_t Size)
{
	if (Size <= PageGranularity) return PageGranularity;
	size_t NewSize = (Size / PageGranularity + 1) * PageGranularity;
	return NewSize;
}

