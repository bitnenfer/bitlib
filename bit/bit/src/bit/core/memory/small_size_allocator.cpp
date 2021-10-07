#include <bit/core/memory/small_size_allocator.h>

bit::SmallSizeAllocator::SmallSizeAllocator() :
	BaseVirtualAddress(nullptr),
	BaseVirtualAddressOffset(0),
	PageFreeList(nullptr),
	PageList(nullptr),
	DecommittedFreeList(nullptr),
	AllocatedBytes(0),
	CommittedBytes(0),
	FreePageSize(0)
{
	BIT_ASSERT(VirtualReserveSpace((void*)0x80000000ULL, TOTAL_ADDRESS_SPACE_SIZE, Memory));
	Memory.CommitPagesByOffset(0, SIZE_OF_BOOKKEEPING);
	PageList = (PageLink*)Memory.GetBaseAddress();
	Memset(PageList, 0, SIZE_OF_BOOKKEEPING);
	Memset(Blocks, 0, sizeof(Blocks));
	BaseVirtualAddress = AlignPtr(OffsetPtr(PageList, SIZE_OF_BOOKKEEPING), PAGE_SIZE);
	CommittedBytes += SIZE_OF_BOOKKEEPING;
}

void* bit::SmallSizeAllocator::Allocate(size_t Size, size_t Alignment)
{
	size_t AlignedSize = AlignUint(Size, Max(MIN_ALLOCATION_SIZE, Alignment));
	size_t BlockIndex = SelectBlockIndex(AlignedSize);
	void* Result = AllocateFreeBlock(AlignedSize, BlockIndex);
	return Result;
}

void bit::SmallSizeAllocator::Free(void* Pointer)
{
	size_t PageIndex = GetPageAllocationInfoIndex(Pointer);
	int64_t BlockSize = PageList[PageIndex].AssignedSize;
	PushPointerToPageFreeList(&PageList[PageIndex], Pointer, (size_t)BlockSize);
	RecordAllocation(SelectBlockIndex(BlockSize), PageIndex, -BlockSize);
}

size_t bit::SmallSizeAllocator::GetSize(void* Pointer)
{
	if (OwnsAllocation(Pointer))
	{
		size_t Index = GetPageAllocationInfoIndex(Pointer);
		PageLink& Info = PageList[Index];
		return Info.AssignedSize;
	}
	return 0;
}

bit::AllocatorMemoryInfo bit::SmallSizeAllocator::GetMemoryUsageInfo()
{
	AllocatorMemoryInfo Usage = {};
	Usage.AllocatedBytes = AllocatedBytes;
	Usage.CommittedBytes = CommittedBytes;
	return Usage;
}

bool bit::SmallSizeAllocator::CanAllocate(size_t Size, size_t Alignment)
{
	return AlignUint(Size, Max(MIN_ALLOCATION_SIZE, Alignment)) <= MAX_ALLOCATION_SIZE;
}

bool bit::SmallSizeAllocator::OwnsAllocation(const void* Ptr)
{
	return PtrInRange(Ptr, Memory.GetBaseAddress(), OffsetPtr(Memory.GetBaseAddress(), USABLE_ADDRESS_SPACE_SIZE));
}

size_t bit::SmallSizeAllocator::Compact()
{
	size_t Total = 0;
	for (FreePageLink* Page = PageFreeList; Page != nullptr;)
	{
		size_t PageIndex = GetPageAllocationInfoIndex(Page);
		PageList[PageIndex].FreeList = nullptr;
		PageList[PageIndex].Prev = nullptr;
		PageList[PageIndex].Next = nullptr;
		PageList[PageIndex].AssignedSize = 0;
		PageList[PageIndex].AllocatedBytes = 0;
		FreePageLink* Next = Page->Next;
		Memory.DecommitPagesByAddress(Page, PAGE_SIZE);
		Page = Next;
		Total += PAGE_SIZE;
		PushPageToDecommitFreeList(&PageList[PageIndex]);
	}
	PageFreeList = nullptr;
	CommittedBytes -= Total;
	FreePageSize = 0;
	return Total;
}

void bit::SmallSizeAllocator::PushPageToDecommitFreeList(PageLink* Info)
{
	Info->Next = DecommittedFreeList;
	DecommittedFreeList = Info;
	Info->Prev = nullptr;
}

bit::SmallSizeAllocator::PageLink* bit::SmallSizeAllocator::PopPageFromDecommitFreeList()
{
	PageLink* AvailablePage = DecommittedFreeList;
	if (AvailablePage != nullptr)
	{
		DecommittedFreeList = AvailablePage->Next;
	}
	return AvailablePage;
}

size_t bit::SmallSizeAllocator::GetBlockSize(size_t BlockIndex)
{
	return (BlockIndex + 1) * MIN_ALLOCATION_SIZE;
}

size_t bit::SmallSizeAllocator::SelectBlockIndex(size_t Size)
{
	if (Size <= MIN_ALLOCATION_SIZE) return 0;
	else if ((Size % MIN_ALLOCATION_SIZE) == 0) return Size / MIN_ALLOCATION_SIZE - 1;
	return ((Size / MIN_ALLOCATION_SIZE + 1) * MIN_ALLOCATION_SIZE) / MIN_ALLOCATION_SIZE - 1;
}

void* bit::SmallSizeAllocator::GetPageBaseByAddress(const void* Address)
{
	uintptr_t PageBase = (uintptr_t)Address & ~(PAGE_SIZE - 1);
	return (void*)PageBase;
}

size_t bit::SmallSizeAllocator::GetPageAllocationInfoIndex(const void* Address)
{
	uintptr_t Base = (uintptr_t)BaseVirtualAddress;
	uintptr_t PageBase = (uintptr_t)GetPageBaseByAddress(Address);
	uintptr_t Offset = PageBase - Base;
	return Offset / PAGE_SIZE;
}

bit::SmallSizeAllocator::PageLink* bit::SmallSizeAllocator::GetPageAllocationInfo(const void* Address)
{
	return &PageList[GetPageAllocationInfoIndex(Address)];
}

void* bit::SmallSizeAllocator::GetPageFromAllocationInfo(PageLink* PageInfoPtr)
{
	uintptr_t Diff = (uintptr_t)PageInfoPtr - (uintptr_t)PageList;
	uintptr_t Index = Diff / sizeof(PageLink);
	return OffsetPtr(BaseVirtualAddress, Index * PAGE_SIZE);
}

void bit::SmallSizeAllocator::UnlinkPage(size_t BlockIndex, size_t PageIndex)
{
	PageLink* PageInfo = &PageList[PageIndex];
	if (Blocks[BlockIndex].Pages == PageInfo)
	{
		Blocks[BlockIndex].Pages = PageInfo->Next;
	}
	if (PageInfo->Prev != nullptr) PageInfo->Prev->Next = PageInfo->Next;
	if (PageInfo->Next != nullptr) PageInfo->Next->Prev = PageInfo->Prev;
	PageInfo->AssignedSize = 0;
}

void bit::SmallSizeAllocator::PushPageToFreeList(size_t PageIndex)
{
	FreePageLink* Page = (FreePageLink*)GetPageAddress(PageIndex);
	Page->Next = PageFreeList;
	PageFreeList = Page;
	FreePageSize += PAGE_SIZE;
	if (FreePageSize >= MIN_DECOMMIT_SIZE)
	{
		Compact();
	}
}

void bit::SmallSizeAllocator::RecordAllocation(size_t BlockIndex, size_t PageIndex, int64_t Size)
{
	PageLink& Info = PageList[PageIndex];
	BIT_ASSERT(Info.AssignedSize > 0);
	Info.AllocatedBytes += Size;
	AllocatedBytes += Size;
	Blocks[BlockIndex].AllocatedBytes += Size;
	if (Info.AllocatedBytes == 0)
	{
		UnlinkPage(BlockIndex, PageIndex);
		PushPageToFreeList(PageIndex);
	}
}

void* bit::SmallSizeAllocator::AllocateFreeBlock(size_t BlockSize, size_t BlockIndex)
{
	void* Block = PopFreeBlock(BlockIndex);
	if (Block == nullptr)
	{
		Block = AllocateFromNewPage(BlockSize, BlockIndex);
	}
	BIT_ASSERT(Block != nullptr);
	RecordAllocation(BlockIndex, GetPageAllocationInfoIndex(Block), BlockSize);
#if SMALL_SIZE_ALLOCATOR_MARK_BLOCKS
	Memset(Block, 0xAA, BlockSize);
#endif
	return Block;
}

void* bit::SmallSizeAllocator::GetPageAddress(size_t PageIndex)
{
	return OffsetPtr(BaseVirtualAddress, PageIndex * PAGE_SIZE);
}

void* bit::SmallSizeAllocator::AllocateNewPage(size_t BlockSize)
{
	BIT_ASSERT(BlockSize > 0);

	if (PageFreeList != nullptr)
	{
		FreePageLink* Pages = PageFreeList;
		PageFreeList = Pages->Next;
		size_t BookkeepingIndex = GetPageAllocationInfoIndex(Pages);
		PageList[BookkeepingIndex].AllocatedBytes = 0;
		PageList[BookkeepingIndex].AssignedSize = BlockSize;
		PageList[BookkeepingIndex].Next = nullptr;
		PageList[BookkeepingIndex].Prev = nullptr;
		PageList[BookkeepingIndex].FreeList = nullptr;
		return Pages;
	}

	if (DecommittedFreeList != nullptr)
	{
		PageLink* AvailablePage = PopPageFromDecommitFreeList();
		AvailablePage->AllocatedBytes = 0;
		AvailablePage->AssignedSize = BlockSize;
		AvailablePage->Next = nullptr;
		AvailablePage->Prev = nullptr;
		AvailablePage->FreeList = nullptr;
		CommittedBytes += PAGE_SIZE;
		void* Pages = GetPageFromAllocationInfo(AvailablePage);
		Memory.CommitPagesByAddress(Pages, PAGE_SIZE);
		return Pages;
	}

	BIT_ASSERT_MSG(BaseVirtualAddressOffset + PAGE_SIZE <= USABLE_ADDRESS_SPACE_SIZE, "Out of virtual address space");

	size_t BookkeepingIndex = (BaseVirtualAddressOffset) / PAGE_SIZE;
	void* Pages = OffsetPtr(BaseVirtualAddress, BaseVirtualAddressOffset);
	Memory.CommitPagesByAddress(Pages, PAGE_SIZE);
	BaseVirtualAddressOffset += PAGE_SIZE;
	CommittedBytes += PAGE_SIZE;
	PageList[BookkeepingIndex].AllocatedBytes = 0;
	PageList[BookkeepingIndex].AssignedSize = BlockSize;
	PageList[BookkeepingIndex].Next = nullptr;
	PageList[BookkeepingIndex].Prev = nullptr;
	PageList[BookkeepingIndex].FreeList = nullptr;
	return Pages;
}

void bit::SmallSizeAllocator::SwapPages(PageLink* A, PageLink* B)
{
	PageLink* ANext = A->Next;
	PageLink* APrev = A->Prev;
	A->Next = B->Next;
	A->Prev = B->Prev;
	B->Next = ANext;
	B->Prev = APrev;
}

void* bit::SmallSizeAllocator::PopFreeBlock(size_t BlockIndex)
{
	PageLink* Pages = Blocks[BlockIndex].Pages;
	if (Pages != nullptr)
	{
	RetryPopFreeBlock:
		if (Pages->FreeList != nullptr)
		{
			FreeBlockLink* Block = Pages->FreeList;
			Pages->FreeList = Block->Next;
			Block->Next = nullptr;
			return Block;
		}
		// Swap pages with free list
		for (PageLink* AvailablePage = Pages->Next; AvailablePage != nullptr; AvailablePage = AvailablePage->Next)
		{
			if (AvailablePage->FreeList != nullptr)
			{
				SwapPages(Pages, AvailablePage);
				Blocks[BlockIndex].Pages = AvailablePage;
				Pages = AvailablePage;
				goto RetryPopFreeBlock;
				break;
			}
		}
	}
	return nullptr;
}

void bit::SmallSizeAllocator::PushFreeBlockToPageFreeList(PageLink* PageInfo, FreeBlockLink* Block, size_t BlockIndex)
{
#if SMALL_SIZE_ALLOCATOR_MARK_BLOCKS
	Memset(Block, 0xFF, GetBlockSize(BlockIndex));
#endif
	FreeBlockLink* FreeListHead = PageInfo->FreeList;
	Block->Next = FreeListHead;
	PageInfo->FreeList = Block;
}

void bit::SmallSizeAllocator::PushPointerToPageFreeList(PageLink* PageInfo, void* Pointer, size_t Size)
{
	PushFreeBlockToPageFreeList(PageInfo, (FreeBlockLink*)Pointer, SelectBlockIndex(Size));
}

void bit::SmallSizeAllocator::PushPageToBlockPageList(PageLink* PageInfo, size_t BlockIndex)
{
	PageLink* PageHead = Blocks[BlockIndex].Pages;
	if (PageHead != nullptr) PageHead->Prev = PageInfo;
	PageInfo->Next = PageHead;
	PageInfo->Prev = nullptr;
	Blocks[BlockIndex].Pages = PageInfo;
}

void bit::SmallSizeAllocator::PopPageFromBlockPageList(PageLink* PageInfo, size_t BlockIndex)
{
	Blocks[BlockIndex].Pages = PageInfo->Next;
	PageInfo->Next = nullptr;
}

void bit::SmallSizeAllocator::SetPageFreeList(void* Pages, size_t BlockSize, size_t BlockIndex)
{
	PageLink* PageInfo = GetPageAllocationInfo(Pages);
	size_t PageWastage = PAGE_SIZE % BlockSize;
	size_t BlockCount = PAGE_SIZE / BlockSize;
	for (int32_t Index = 0; Index < BlockCount; ++Index)
	{
		FreeBlockLink* Block = OffsetPtr<FreeBlockLink>(Pages, Index * BlockSize);
		PushFreeBlockToPageFreeList(PageInfo, Block, BlockIndex);
	}
	PushPageToBlockPageList(PageInfo, BlockIndex);
}

void* bit::SmallSizeAllocator::AllocateFromNewPage(size_t BlockSize, size_t BlockIndex)
{
	void* NewPage = AllocateNewPage(BlockSize);
	SetPageFreeList(NewPage, BlockSize, BlockIndex);
	return PopFreeBlock(BlockIndex);
}
