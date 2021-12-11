#include <bit/core/memory/small_size_allocator.h>
#include <bit/core/os/atomics.h>

bit::SmallSizeAllocator::SmallSizeAllocator() :
	PageDecommitList(nullptr),
	PageFreeList(nullptr),
	BaseVirtualAddress(nullptr),
	BaseVirtualAddressOffset(0),
	PageFreeListBytes(0),
	AllocatedBytes(0),
	CommittedBytes(0)
{
	VirtualReserveSpace((void*)0x80000000ULL, ADDRESS_SPACE_SIZE, Memory);
	BIT_ASSERT(Memory.GetBaseAddress() != nullptr);
	Memset(Blocks, 0, sizeof(Blocks));
	Memset(Pages, 0, sizeof(Pages));
	BaseVirtualAddress = Memory.GetBaseAddress();
	for (int64_t Index = 0; Index < NUM_OF_PAGES; ++Index)
	{
		Pages[Index].PageIndex = Index;
	}
}

void* bit::SmallSizeAllocator::Allocate(size_t Size, size_t Alignment)
{
	size_t AlignedSize = AlignUint(Size, Max(MIN_ALLOCATION_SIZE, Alignment));
	size_t BlockIndex = GetBlockIndex(AlignedSize);
	FreeBlockLink* Block = GetFreeBlock(BlockIndex);
	if (Block == nullptr)
	{
		Block = GetFreeBlockFromNewPage(BlockIndex);
	}
	BIT_ASSERT(OwnsAllocation(Block));
	BIT_ASSERT(Block != nullptr);
	OnAlloc(BlockIndex, GetPageDataIndex(Block));
	return Block;
}

void bit::SmallSizeAllocator::Free(void* Pointer)
{
	if (OwnsAllocation(Pointer))
	{
		size_t BlockSize = GetPageData(Pointer)->AssignedSize;
		size_t BlockIndex = GetBlockIndex(BlockSize);
		FreeBlockLink* FreeBlock = reinterpret_cast<FreeBlockLink*>(Pointer);
		PushBlockToFreeList(BlockIndex, FreeBlock);
		OnFree(BlockIndex, GetPageDataIndex(Pointer));
	}
}

size_t bit::SmallSizeAllocator::GetSize(void* Pointer)
{
	if (Pointer != nullptr)
	{
		return GetPageData(Pointer)->AssignedSize;
	}
	return 0;
}

bit::AllocatorMemoryInfo bit::SmallSizeAllocator::GetMemoryUsageInfo()
{
	AllocatorMemoryInfo Info = {};
	Info.AllocatedBytes = AllocatedBytes;
	Info.CommittedBytes = CommittedBytes;
	return Info;
}

bool bit::SmallSizeAllocator::CanAllocate(size_t Size, size_t Alignment)
{
	return AlignUint(Size, Max(MIN_ALLOCATION_SIZE, Alignment)) <= MAX_ALLOCATION_SIZE;
}

bool bit::SmallSizeAllocator::OwnsAllocation(const void* Ptr)
{
	return PtrInRange(Ptr, Memory.GetBaseAddress(), OffsetPtr(Memory.GetBaseAddress(), ADDRESS_SPACE_SIZE));
}

size_t bit::SmallSizeAllocator::Compact()
{
	return DecommitFreePages();
}

bit::SmallSizeAllocator::FreeBlockLink* bit::SmallSizeAllocator::UnlinkFreeBlock(size_t BlockIndex, FreeBlockLink* FreeBlock)
{
	if (FreeBlock->PrevBlock == nullptr)
	{
		Blocks[BlockIndex].FreeList = FreeBlock->NextBlock;
		if (FreeBlock->NextBlock != nullptr) FreeBlock->NextBlock->PrevBlock = nullptr;
	}
	else
	{
		FreeBlock->PrevBlock->NextBlock = FreeBlock->NextBlock;
		if (FreeBlock->NextBlock != nullptr) FreeBlock->NextBlock->PrevBlock = FreeBlock->PrevBlock;
	}
	FreeBlock->PrevBlock = nullptr;
	FreeBlock->NextBlock = nullptr;
	return FreeBlock;
}

void bit::SmallSizeAllocator::OnAlloc(size_t BlockIndex, size_t PageIndex)
{
	int64_t BlockSize = (int64_t)GetBlockSize(BlockIndex);
	AtomicAdd(&Blocks[BlockIndex].AllocatedBytes, BlockSize);
	AtomicAdd(&Pages[PageIndex].AllocatedBytes, BlockSize);
	AtomicAdd(&AllocatedBytes, BlockSize);
}

void bit::SmallSizeAllocator::OnFree(size_t BlockIndex, size_t PageIndex)
{
	int64_t BlockSize = (int64_t)GetBlockSize(BlockIndex);
	AtomicAdd(&Blocks[BlockIndex].AllocatedBytes, -BlockSize);
	AtomicAdd(&Pages[PageIndex].AllocatedBytes, -BlockSize);
	AtomicAdd(&AllocatedBytes, -BlockSize);

	BIT_ASSERT(Pages[PageIndex].AllocatedBytes >= 0);
	if (Pages[PageIndex].AllocatedBytes == 0)
	{
		FreePage(PageIndex);
	}
}

void bit::SmallSizeAllocator::FreePage(size_t PageIndex)
{
	FreePageLink* Page = (FreePageLink*)GetPageBaseByAddressByIndex(PageIndex);
	PageMetadata* PageData = &Pages[PageIndex];
	size_t BlockSize = PageData->AssignedSize;
	size_t BlockIndex = GetBlockIndex(BlockSize);
	size_t BlockCount = PAGE_SIZE / BlockSize;
	BlockMetadata* BlockData = &Blocks[BlockIndex];
	FreeBlockLink* BlockHead = reinterpret_cast<FreeBlockLink*>(Page);

	for (size_t Index = 0; Index < BlockCount; ++Index)
	{
		UnlinkFreeBlock(BlockIndex, (FreeBlockLink*)OffsetPtr(BlockHead, Index * BlockSize));
	}

	Page->NextPage = PageFreeList;
	PageFreeList = Page;

	AtomicAdd(&PageFreeListBytes, PAGE_SIZE);
	if (PageFreeListBytes >= MIN_DECOMMIT_SIZE)
	{
		DecommitFreePages();
	}
}

size_t bit::SmallSizeAllocator::DecommitFreePages()
{
	int64_t TotalDecommitted = 0;
	for (FreePageLink* FreePage = PageFreeList; FreePage != nullptr; )
	{
		FreePageLink* NextFreePage = FreePage->NextPage;
		Memory.DecommitPagesByAddress(FreePage, PAGE_SIZE);

		PageMetadata* PageData = GetPageData(FreePage);
		PageData->NextFreePage = PageDecommitList;
		PageDecommitList = PageData;

		FreePage = NextFreePage;
		TotalDecommitted += PAGE_SIZE;
	}
	AtomicAdd(&CommittedBytes, -TotalDecommitted);
	AtomicExchange(&PageFreeListBytes, 0);
	PageFreeList = nullptr;
	return TotalDecommitted;
}


size_t bit::SmallSizeAllocator::GetBlockSize(size_t BlockIndex)
{
	return (BlockIndex + 1) * MIN_ALLOCATION_SIZE;
}

void* bit::SmallSizeAllocator::GetPageBaseByAddressByIndex(size_t PageIndex)
{
	return OffsetPtr(BaseVirtualAddress, PageIndex * PAGE_SIZE);
}

void* bit::SmallSizeAllocator::GetPageBaseByAddress(const void* Ptr)
{
	uintptr_t PageBase = (uintptr_t)Ptr & ~(PAGE_SIZE - 1);
	return (void*)PageBase;
}

size_t bit::SmallSizeAllocator::GetPageDataIndex(const void* Ptr)
{
	uintptr_t Base = (uintptr_t)BaseVirtualAddress;
	uintptr_t PageBase = (uintptr_t)GetPageBaseByAddress(Ptr);
	uintptr_t Offset = PageBase - Base;
	return Offset / PAGE_SIZE;
}

bit::SmallSizeAllocator::PageMetadata* bit::SmallSizeAllocator::GetPageData(const void* Ptr)
{
	return &Pages[GetPageDataIndex(Ptr)];
}

size_t bit::SmallSizeAllocator::GetBlockIndex(size_t BlockSize)
{
	if (BlockSize <= MIN_ALLOCATION_SIZE) return 0;
	else if ((BlockSize % MIN_ALLOCATION_SIZE) == 0) return BlockSize / MIN_ALLOCATION_SIZE - 1;
	return ((BlockSize / MIN_ALLOCATION_SIZE + 1) * MIN_ALLOCATION_SIZE) / MIN_ALLOCATION_SIZE - 1;
}

void bit::SmallSizeAllocator::PushBlockToFreeList(size_t BlockIndex, FreeBlockLink* FreeBlock)
{
	if (Blocks[BlockIndex].FreeList != nullptr)
	{
		Blocks[BlockIndex].FreeList->PrevBlock = FreeBlock;
	}
	FreeBlock->NextBlock = Blocks[BlockIndex].FreeList;
	Blocks[BlockIndex].FreeList = FreeBlock;
	FreeBlock->PrevBlock = nullptr;
}

bit::SmallSizeAllocator::FreeBlockLink* bit::SmallSizeAllocator::GetFreeBlock(size_t BlockIndex)
{
	if (Blocks[BlockIndex].FreeList != nullptr)
	{
		FreeBlockLink* FreeBlock = UnlinkFreeBlock(BlockIndex, Blocks[BlockIndex].FreeList);
		return FreeBlock;
	}
	return nullptr;
}

bit::SmallSizeAllocator::FreePageLink* bit::SmallSizeAllocator::AllocateNewPage()
{
	BIT_ASSERT(BaseVirtualAddressOffset + PAGE_SIZE <= ADDRESS_SPACE_SIZE); // Out of virtual address space
	size_t PageDataIndex = (BaseVirtualAddressOffset) / PAGE_SIZE;
	void* Page = OffsetPtr(BaseVirtualAddress, BaseVirtualAddressOffset);
	Memory.CommitPagesByAddress(Page, PAGE_SIZE);
	AtomicAdd(&BaseVirtualAddressOffset, PAGE_SIZE);
	AtomicAdd(&CommittedBytes, PAGE_SIZE);
	Pages[PageDataIndex].AllocatedBytes = 0;
	Pages[PageDataIndex].AssignedSize = 0;
	return reinterpret_cast<FreePageLink*>(Page);
}

bit::SmallSizeAllocator::FreePageLink* bit::SmallSizeAllocator::GetFreePage()
{
	if (PageFreeList != nullptr)
	{
		FreePageLink* FreePage = PageFreeList;
		PageFreeList = FreePage->NextPage;
		AtomicAdd(&PageFreeListBytes, -(int64_t)PAGE_SIZE);
		return FreePage;
	}

	if (PageDecommitList != nullptr)
	{
		PageMetadata* FreePage = PageDecommitList;
		PageDecommitList = FreePage->NextFreePage;
		FreePage->NextFreePage = nullptr;
		FreePageLink* Page = (FreePageLink*)GetPageBaseByAddressByIndex(FreePage->PageIndex);
		Memory.CommitPagesByAddress(Page, PAGE_SIZE);
		AtomicAdd(&CommittedBytes, PAGE_SIZE);
		return Page;
	}

	return AllocateNewPage();
}

bit::SmallSizeAllocator::FreeBlockLink* bit::SmallSizeAllocator::GetFreeBlockFromNewPage(size_t BlockIndex)
{
	FreePageLink* Page = GetFreePage();
	PageMetadata* PageData = GetPageData(Page);
	FreeBlockLink* BlockHead = reinterpret_cast<FreeBlockLink*>(Page);
	size_t BlockSize = GetBlockSize(BlockIndex);
	size_t BlockCount = PAGE_SIZE / BlockSize;

	for (size_t Index = 0; Index < BlockCount; ++Index)
	{
		PushBlockToFreeList(BlockIndex, (FreeBlockLink*)OffsetPtr(BlockHead, Index * BlockSize));
	}

	PageData->AssignedSize = BlockSize;
	return GetFreeBlock(BlockIndex);
}
