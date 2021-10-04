#include <bit/core/memory/small_size_allocator.h>
#include <bit/core/memory.h>
#include <bit/core/os/debug.h>

bit::SmallSizeAllocator::SmallSizeAllocator() :
	BaseVirtualAddress(nullptr),
	BaseVirtualAddressOffset(0),
	PageFreeList(nullptr),
	PageInfo(nullptr)
{
	if (!VirtualReserveSpace(nullptr, TOTAL_ADDRESS_SPACE_SIZE, Memory))
	{
		BIT_PANIC();
	}
	PageInfo = (PageAllocationInfo*)Memory.CommitPagesByOffset(0, SIZE_OF_BOOKKEEPING);
	bit::Memset(PageInfo, 0, SIZE_OF_BOOKKEEPING);
	bit::Memset(Blocks, 0, sizeof(Blocks));
	BaseVirtualAddress = bit::AlignPtr(bit::OffsetPtr(PageInfo, SIZE_OF_BOOKKEEPING), PAGE_SIZE);
}

void* bit::SmallSizeAllocator::Allocate(size_t Size, size_t Alignment)
{
	size_t AlignedSize = bit::AlignUint(Size, bit::Max(MIN_ALLOCATION_SIZE, Alignment));
	size_t BlockIndex = SelectBlockIndex(AlignedSize);
	return AllocateFreeBlock(AlignedSize, BlockIndex);
}

void bit::SmallSizeAllocator::Free(void* Pointer)
{
	if (OwnsAllocation(Pointer))
	{
		size_t PageIndex = GetPageAllocationInfoIndex(Pointer);
		int64_t BlockSize = PageInfo[PageIndex].AssignedSize;
		PushPointerToFreeList(Pointer, (size_t)BlockSize);
		TrackAllocation(SelectBlockIndex(BlockSize), PageIndex, -BlockSize);
	}
}

size_t bit::SmallSizeAllocator::GetSize(void* Pointer)
{
	if (OwnsAllocation(Pointer))
	{
		PageAllocationInfo& Info = PageInfo[GetPageAllocationInfoIndex(Pointer)];
		return Info.AssignedSize;
	}
	return 0;
}

bit::AllocatorMemoryInfo bit::SmallSizeAllocator::GetMemoryUsageInfo()
{
	AllocatorMemoryInfo Info = { };
	Info.CommittedBytes = Memory.GetCommittedSize();
	Info.ReservedBytes = Memory.GetReservedSize();
	Info.AllocatedBytes = (size_t)AllocatedBytes;
	return Info;
}

bool bit::SmallSizeAllocator::CanAllocate(size_t Size, size_t Alignment)
{
	return bit::AlignUint(Size, bit::Max(MIN_ALLOCATION_SIZE, Alignment)) <= MAX_ALLOCATION_SIZE;
}

bool bit::SmallSizeAllocator::OwnsAllocation(const void* Ptr)
{
	return Memory.OwnsAddress(Ptr);
}

size_t bit::SmallSizeAllocator::Compact()
{
	size_t Total = 0;
	for (FreePage* Page = PageFreeList; Page != nullptr;)
	{
		size_t PageIndex = GetPageAllocationInfoIndex(Page);
		PageInfo[PageIndex].bCommitted = false;
		PageInfo[PageIndex].AssignedSize = 0;
		PageInfo[PageIndex].AllocatedBytes = 0;
		FreePage* Next = Page->Next;
		Memory.DecommitPagesByAddress(Page, PAGE_SIZE);
		Page = Next;
		Total += PAGE_SIZE;
	}
	return Total;
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

void bit::SmallSizeAllocator::TrackAllocation(size_t BlockIndex, size_t PageIndex, int64_t Size)
{
	PageAllocationInfo& Info = PageInfo[PageIndex];
	Info.AllocatedBytes += Size;
	AllocatedBytes += Size;
	Blocks[BlockIndex].AllocatedBytes += Size;
	if (Info.AllocatedBytes == 0)
	{
		size_t BlockSize = GetBlockSize(BlockIndex);
		size_t BlockCount = PAGE_SIZE / BlockSize;
		void* PageAddress = GetPageAddress(PageIndex);
		for (size_t Index = 0; Index < BlockCount; ++Index)
		{
			FreeBlock* Block = (FreeBlock*)bit::OffsetPtr(PageAddress, BlockSize * Index);
			UnlinkFreeBlock(Block, Blocks[BlockIndex]);
		}
		FreePage* Page = (FreePage*)PageAddress;
		Page->Next = PageFreeList;
		PageFreeList = Page;
		Info.AssignedSize = 0;
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
	TrackAllocation(BlockIndex, GetPageAllocationInfoIndex(Block), BlockSize);
	return Block;
}

void* bit::SmallSizeAllocator::GetPageAddress(size_t PageIndex)
{
	return bit::OffsetPtr(BaseVirtualAddress, PageIndex * PAGE_SIZE);
}

void* bit::SmallSizeAllocator::AllocateNewPage(size_t BlockSize)
{
	if (PageFreeList != nullptr)
	{
		FreePage* Page = PageFreeList;
		PageFreeList = Page->Next;
		return Page;
	}

	size_t BookkeepingIndex = BaseVirtualAddressOffset / PAGE_SIZE;
	void* Page = bit::OffsetPtr(BaseVirtualAddress, BaseVirtualAddressOffset);
	Page = Memory.CommitPagesByAddress(Page, PAGE_SIZE);
	BaseVirtualAddressOffset += PAGE_SIZE;
	PageInfo[BookkeepingIndex].AllocatedBytes = 0;
	PageInfo[BookkeepingIndex].AssignedSize = (int16_t)BlockSize;
	PageInfo[BookkeepingIndex].bCommitted = true;
	return Page;
}

void* bit::SmallSizeAllocator::AllocateFromNewPage(size_t BlockSize, size_t BlockIndex)
{
	void* NewPage = AllocateNewPage(BlockSize);
	PushPageToFreeBlocks(NewPage, BlockSize, BlockIndex);
	return PopFreeBlock(BlockIndex);
}

void bit::SmallSizeAllocator::UnlinkFreeBlock(FreeBlock* Block, BlockInfo& Info)
{
	if (Block != nullptr)
	{
	#if BIT_SMALL_SIZE_ALLOCATOR_USE_LARGE_BLOCKS
		if (Block->Prev == nullptr) Info.FreeList = Block->Next;
		if (Block->Prev != nullptr) Block->Prev->Next = Block->Next;
		if (Block->Next != nullptr) Block->Next->Prev = Block->Prev;
		Block->Prev = nullptr;
		Block->Next = nullptr;
	#else
		for (FreeBlock* CurrBlock = Info.FreeList, *Prev = nullptr; CurrBlock != nullptr;)
		{
			if (CurrBlock == Block)
			{
				if (Prev != nullptr) Prev->Next = Block->Next;
				else Info.FreeList = Block->Next;
				Block->Next = nullptr;
				break;
			}

			Prev = CurrBlock;
			CurrBlock = CurrBlock->Next;
		}
	#endif
	}
}

void* bit::SmallSizeAllocator::PopFreeBlock(size_t BlockIndex)
{
	if (Blocks[BlockIndex].FreeList != nullptr)
	{
		FreeBlock* Block = Blocks[BlockIndex].FreeList;
		FreeBlock* Next = Block->Next;
	#if BIT_SMALL_SIZE_ALLOCATOR_USE_LARGE_BLOCKS
		if (Next != nullptr) Next->Prev = nullptr;
	#endif
		Blocks[BlockIndex].FreeList = Next;
		Block->Next = nullptr;
		return Block;
	}
	return nullptr;
}

void bit::SmallSizeAllocator::PushFreeBlockToFreeList(FreeBlock* Block, size_t BlockIndex)
{
	FreeBlock* FreeListHead = Blocks[BlockIndex].FreeList;
#if BIT_SMALL_SIZE_ALLOCATOR_USE_LARGE_BLOCKS
	if (FreeListHead != nullptr) FreeListHead->Prev = Block;
	Block->Prev = nullptr;
#endif
	Block->Next = FreeListHead;
	Blocks[BlockIndex].FreeList = Block;
}

void bit::SmallSizeAllocator::PushPointerToFreeList(void* Pointer, size_t Size)
{
	PushFreeBlockToFreeList((FreeBlock*)Pointer, SelectBlockIndex(Size));
}

void bit::SmallSizeAllocator::PushPageToFreeBlocks(void* Pages, size_t BlockSize, size_t BlockIndex)
{
	size_t BlockCount = PAGE_SIZE / BlockSize;
	for (int32_t Index = (int32_t)BlockCount - 1; Index >= 0; --Index)
	{
		FreeBlock* Block = bit::OffsetPtr<FreeBlock>(Pages, Index * BlockSize);
		PushFreeBlockToFreeList(Block, BlockIndex);
	}
}


