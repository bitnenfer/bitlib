#include <bit/core/memory/medium_size_allocator.h>
#include <bit/core/memory.h>
#include <bit/utility/scope_lock.h>
#include <bit/core/os/debug.h>
#include <bit/core/os/os.h>

#define BIT_ENABLE_BLOCK_MARKING 0

bit::MediumSizeAllocator::MediumSizeAllocator() :
	FLBitmap(0),
	PagesInUse(nullptr),
	VirtualMemoryBaseAddress(nullptr),
	PagesInUseCount(0),
	VirtualMemoryBaseOffset(0),
	UsedSpaceInBytes(0),
	AvailableSpaceInBytes(0)
{
	if (!VirtualReserveSpace((void*)nullptr, ADDRESS_SPACE_SIZE, Memory))
	{
		BIT_PANIC();
	}
	VirtualMemoryBaseOffset = 0;
	VirtualMemoryBaseAddress = Memory.GetBaseAddress();
	bit::Memset(SLBitmap, 0, sizeof(SLBitmap));
	bit::Memset(FreeBlocks, 0, sizeof(FreeBlocks));
}

bit::MediumSizeAllocator::~MediumSizeAllocator()
{
	VirtualReleaseSpace(Memory);
}

void* bit::MediumSizeAllocator::Allocate(size_t Size, size_t Alignment)
{
	uint64_t RoundedSize = RoundToSlotSize(Size);
	uint64_t AdjustedSize = (uint64_t)bit::Max((uint64_t)RoundedSize, MIN_ALLOCATION_SIZE);
	if (FLBitmap == 0) 
		AllocateVirtualMemory(AdjustedSize);

	if (Alignment > alignof(BlockHeader))
	{
		return AllocateAligned(AdjustedSize, (uint64_t)Alignment);
	}

	uint64_t AlignedSize = (uint64_t)bit::AlignUint(AdjustedSize, alignof(BlockHeader));
	BlockMap Map = Mapping(AlignedSize);
	BlockFreeHeader* Block = FindSuitableBlock(AdjustedSize, Map);
	if (Block == nullptr)
	{
		AllocateVirtualMemory(AdjustedSize);
		Block = FindSuitableBlock(AlignedSize, Map);
	}
	if (Block != nullptr)
	{
		BIT_ASSERT(!Block->IsLastPhysicalBlock());
		uint64_t BlockSize = Block->GetSize();
		uint64_t BlockFullSize = Block->GetFullSize();
		RemoveBlock(Block, Map);
		if (Block->GetSize() > AlignedSize && (Block->GetSize() - AlignedSize) > MIN_ALLOCATION_SIZE)
		{
			BlockFreeHeader* RemainingBlock = Split(Block, AlignedSize);
			if (RemainingBlock != Block)
			{
				InsertBlock(RemainingBlock, Mapping(RemainingBlock->GetSize()));
			}
		}

		UsedSpaceInBytes += Block->GetSize();
	#if BIT_ENABLE_BLOCK_MARKING
		FillBlock(Block, 0xAA);
	#endif
		return GetPointerFromBlockHeader(Block);
	}
	return nullptr; /* Out of memory */
}

void* bit::MediumSizeAllocator::AllocateAligned(uint64_t Size, uint64_t Alignment)
{
	uint64_t RoundedSize = RoundToSlotSize(Size);
	uint64_t AlignedSize = (uint64_t)bit::AlignUint(RoundedSize, Alignment);
	BlockMap Map = Mapping(AlignedSize);
	BlockFreeHeader* Block = FindSuitableBlock(AlignedSize, Map);
	if (Block == nullptr)
	{
		AllocateVirtualMemory(AlignedSize);
		Block = FindSuitableBlock(AlignedSize, Map);
	}
	if (Block != nullptr)
	{
		uint64_t BlockSize = Block->GetSize();
		uint64_t BlockFullSize = Block->GetFullSize();
		RemoveBlock(Block, Map);
		if (!bit::IsAddressAligned(GetPointerFromBlockHeader(Block), Alignment))
		{
			BlockFreeHeader* AlignedBlock = SplitAligned(Block, RoundedSize, Alignment);
			if (AlignedBlock != Block)
			{
				InsertBlock(Block, Mapping(Block->GetSize()));
			}
			Block = AlignedBlock;
		}
		else if (Block->GetSize() > AlignedSize && (Block->GetSize() - AlignedSize) > MIN_ALLOCATION_SIZE)
		{
			BlockFreeHeader* RemainingBlock = Split(Block, AlignedSize);
			if (RemainingBlock != Block)
			{
				InsertBlock(RemainingBlock, Mapping(RemainingBlock->GetSize()));
			}
		}

		UsedSpaceInBytes += Block->GetSize();
	#if BIT_ENABLE_BLOCK_MARKING
		FillBlock(Block, 0xAA);
	#endif

		return GetPointerFromBlockHeader(Block);
	}
	return nullptr; /* Out of memory */
}

void* bit::MediumSizeAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr) return Allocate(Size, Alignment);
	size_t BlockSize = GetSize(Pointer);
	if (BlockSize == Size) return Pointer;
	void* NewBlock = Allocate(Size, Alignment);
	bit::Memcpy(NewBlock, Pointer, bit::Min(BlockSize, Size));
	Free(Pointer);
	return NewBlock;
}

void bit::MediumSizeAllocator::Free(void* Pointer)
{
	if (Pointer != nullptr)
	{
		BlockFreeHeader* FreeBlock = GetBlockHeaderFromPointer(Pointer);
	#if BIT_ENABLE_BLOCK_MARKING
		FillBlock(FreeBlock, 0xDD);
	#endif
		UsedSpaceInBytes -= FreeBlock->GetSize();
		BlockFreeHeader* MergedBlock = Merge(FreeBlock);

		if (!FreeVirtualMemory(MergedBlock))
		{
			InsertBlock(MergedBlock, Mapping(MergedBlock->GetSize()));
		}
	}
}

size_t bit::MediumSizeAllocator::GetSize(void* Pointer)
{
	if (Pointer != nullptr)
	{
		return GetBlockHeaderFromPointer(Pointer)->GetSize();
	}
	return 0;
}

bit::AllocatorMemoryInfo bit::MediumSizeAllocator::GetMemoryUsageInfo()
{
	AllocatorMemoryInfo Usage = {};
	Usage.CommittedBytes = Memory.GetCommittedSize();
	Usage.ReservedBytes = Memory.GetReservedSize();
	Usage.AllocatedBytes = UsedSpaceInBytes;
	return Usage;
}

bool bit::MediumSizeAllocator::CanAllocate(size_t Size, size_t Alignment)
{
	size_t AlignedSize = bit::AlignUint(Size, bit::Max(Alignment, alignof(BlockHeader)));
	return AlignedSize >= MIN_ALLOCATION_SIZE && AlignedSize < MAX_ALLOCATION_SIZE;
}

bool bit::MediumSizeAllocator::OwnsAllocation(const void* Ptr)
{
	return Memory.OwnsAddress(Ptr);
}

size_t bit::MediumSizeAllocator::Compact()
{
	size_t Total = 0;
	return Total;
}

size_t bit::MediumSizeAllocator::RoundToSlotSize(size_t Size)
{
	BlockMap Map = MappingNoOffset(Size);
	size_t MinSize = 1ULL << Map.FL;
	size_t MaxSize = bit::NextPow2(MinSize+1);
	size_t RangeDiff = MaxSize - MinSize;
	size_t SlotStep = RangeDiff / SL_COUNT;
	size_t RoundedSize = bit::AlignUint(Size, SlotStep);
	return RoundedSize;
}

bit::MediumSizeAllocator::BlockMap bit::MediumSizeAllocator::MappingNoOffset(size_t Size) const
{
	// First level index is the last set bit
	uint64_t FL = bit::BitScanReverse((uint64_t)Size);
	// Second level index is the next rightmost SLI bits
	// For example if SLI is 4, then 460 would result in 
	// 0000000111001100
	//       / |..|	
	//    FL    SL
	// FL = 8, SL = 12
	uint64_t SL = ((Size ^ (1ULL << FL)) >> (FL - SLI));
	return { FL, SL };
}

void bit::MediumSizeAllocator::AllocateVirtualMemory(size_t Size)
{
	size_t PageSize = GetOSPageSize();
	size_t AdjustedSize = Size + sizeof(BlockHeader) + sizeof(BlockFreeHeader) + sizeof(VirtualPage);
	size_t AlignedSize = bit::RoundUp(bit::AlignUint(AdjustedSize, alignof(BlockHeader)), PageSize);
	void* CommittedPages = Memory.CommitPagesByAddress(bit::OffsetPtr(VirtualMemoryBaseAddress, VirtualMemoryBaseOffset), AlignedSize);
	if (CommittedPages != nullptr)
	{
		VirtualMemoryBaseOffset += AlignedSize;

		VirtualPage* PageInfo = reinterpret_cast<VirtualPage*>(CommittedPages);
		PageInfo->PageSize = AlignedSize;
		PageInfo->Next = PagesInUse;
		PageInfo->Prev = nullptr;
		PageInfo->BlockHead.Reset(0);
		PageInfo->BlockHead.SetLastPhysicalBlock();

		if (PagesInUse != nullptr) PagesInUse->Prev = PageInfo;
		PagesInUse = PageInfo;

		void* Offset = bit::OffsetPtr(PageInfo, sizeof(VirtualPage));
		BlockFreeHeader* Block = reinterpret_cast<BlockFreeHeader*>(bit::AlignPtr(Offset, alignof(BlockHeader)));
		void* StartPtr = GetPointerFromBlockHeader(Block);
		void* EndPtr = bit::OffsetPtr(CommittedPages, AlignedSize);
		size_t BlockSize = bit::PtrDiff(EndPtr, StartPtr) - sizeof(BlockFreeHeader);
		size_t BlockRoundedSize = RoundToSlotSize(BlockSize);
		Block->Reset(BlockSize);
		Block->PrevPhysicalBlock = &PageInfo->BlockHead;

		BlockFreeHeader* EndOfBlock = GetNextBlock(Block);
		EndOfBlock->Reset(0);
		EndOfBlock->SetLastPhysicalBlock();
		EndOfBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Block);
		EndOfBlock->NextFree = nullptr;
		EndOfBlock->PrevFree = nullptr;

		InsertBlock(Block, Mapping(Block->GetSize()));

		BIT_ASSERT(Block->GetSize() < MAX_ALLOCATION_SIZE);
		BIT_ASSERT(Block->GetSize() >= Size);

		AvailableSpaceInBytes += Block->GetSize();
		PagesInUseCount += 1;
	}
}

bool bit::MediumSizeAllocator::FreeVirtualMemory(BlockFreeHeader* FreeBlock)
{
	if (FreeBlock->PrevPhysicalBlock->IsLastPhysicalBlock() && GetNextBlock(FreeBlock)->IsLastPhysicalBlock())
	{
		VirtualPage* Page = reinterpret_cast<VirtualPage*>(FreeBlock->PrevPhysicalBlock);
		if (Page->Prev != nullptr) Page->Prev->Next = Page->Next;
		else PagesInUse = Page->Next;

		if (Page->Next != nullptr) Page->Next->Prev = Page->Prev;

		if (Memory.DecommitPagesByAddress(Page, Page->PageSize))
		{
			PagesInUseCount -= 1;
		}
		return true;
	}
	return false;
}

bit::MediumSizeAllocator::BlockMap bit::MediumSizeAllocator::Mapping(size_t Size) const
{
	BlockMap Map = MappingNoOffset(Size);
	Map.FL -= COUNT_OFFSET - 1; // We need to adjust index so it maps to min alloc size range to max alloc size
	return Map;
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::GetBlockHeaderFromPointer(void* Block) const
{
	return bit::OffsetPtr<BlockFreeHeader>(Block, -(intptr_t)sizeof(BlockHeader));
}

void* bit::MediumSizeAllocator::GetPointerFromBlockHeader(BlockHeader* Block) const
{
	return bit::OffsetPtr(Block, sizeof(BlockHeader));
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::Merge(BlockFreeHeader* Block)
{
	return MergeRecursive(Block);
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::MergeRecursive(BlockFreeHeader* Block)
{
	BlockFreeHeader* Next = GetNextBlock(Block);
	if (Next->GetSize() > 0 && Next->IsFree())
	{
		BlockMap Map = Mapping(Next->GetSize());
		RemoveBlock(Next, Map);
		BlockFreeHeader* MergedBlock = MergeBlocks(Block, Next);
		return MergeRecursive(MergedBlock);
	}

	BlockFreeHeader* Prev = reinterpret_cast<BlockFreeHeader*>(Block->PrevPhysicalBlock);
	if (!Prev->IsLastPhysicalBlock() && Prev->IsFree())
	{
		BlockMap Map = Mapping(Prev->GetSize());
		RemoveBlock(Prev, Map);
		BlockFreeHeader* MergedBlock = MergeBlocks(Prev, Block);
		return MergeRecursive(MergedBlock);
	}

	return Block;
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::FindSuitableBlock(size_t Size, BlockMap& Map)
{
	uint64_t FL = Map.FL;
	uint64_t SL = Map.SL;
	uint64_t SLMapping = SLBitmap[FL] & (~(uint64_t)0 << SL);

RetrySearch:
	if (SLMapping == 0)
	{
		uint64_t FLMapping = FLBitmap & (~(uint64_t)0 << (FL + 1));
		if (FLMapping == 0)
		{
			// OOM - We need to allocate a new pool
			return nullptr;
		}
		FL = bit::BitScanForward(FLMapping);
		SLMapping = SLBitmap[FL];
	}
	SL = bit::BitScanForward(SLMapping);
	BlockFreeHeader* Block = FreeBlocks[FL][SL];
	if (Block->GetSize() < Size)
	{
		// Hitting this point is death. We need to iterate over
		// the free list until we find a block that meets the size requirement
		while (Block != nullptr)
		{
			if (Block->GetSize() >= Size)
				break;
			Block = Block->NextFree;
		}
		if (Block == nullptr)
		{
			SLMapping = 0;
			goto RetrySearch;
		}
	}
	Map.FL = FL;
	Map.SL = SL;
	BIT_ASSERT(Block != nullptr);
	BIT_ASSERT(Block->GetSize() >= Size);
	return Block;
}

void bit::MediumSizeAllocator::RemoveBlock(BlockFreeHeader* Block, BlockMap Map)
{
	Block->SetUsed();
	BlockFreeHeader* Prev = Block->PrevFree;
	BlockFreeHeader* Next = Block->NextFree;
	if (Prev != nullptr) Prev->NextFree = Next;
	if (Next != nullptr) Next->PrevFree = Prev;
	Block->PrevFree = nullptr;
	Block->NextFree = nullptr;

	if (FreeBlocks[Map.FL][Map.SL] == Block)
	{
		FreeBlocks[Map.FL][Map.SL] = Next;
		if (Next == nullptr)
		{
			SLBitmap[Map.FL] &= ~Pow2(Map.SL);
			if (SLBitmap[Map.FL] == 0)
			{
				FLBitmap &= ~Pow2(Map.FL);
			}
		}
	}
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::Split(BlockFreeHeader* Block, uint64_t Size)
{
	uint64_t BlockSize = Block->GetSize();
	uint64_t FullBlockSize = Block->GetFullSize();
	uint64_t SizeWithHeader = Size + sizeof(BlockHeader);
	if (Size >= MIN_ALLOCATION_SIZE && BlockSize - SizeWithHeader >= MIN_ALLOCATION_SIZE)
	{
		BlockFreeHeader* RemainingBlock = bit::OffsetPtr<BlockFreeHeader>(Block, SizeWithHeader);
		RemainingBlock->Reset(BlockSize - SizeWithHeader);
		RemainingBlock->SetUsed();
		RemainingBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Block);
		BlockFreeHeader* NextBlock = GetNextBlock(RemainingBlock);
		NextBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(RemainingBlock);
		Block->SetSize(Size);
		BIT_ASSERT(FullBlockSize == Block->GetFullSize() + RemainingBlock->GetFullSize());
		//BIT_ASSERT(RoundToSlotSize(Block->GetSize()) == Block->GetSize());
		return RemainingBlock;
	}
	return Block;
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::SplitAligned(BlockFreeHeader* Block, uint64_t Size, uint64_t Alignment)
{
	uint64_t BlockSize = Block->GetSize();
	uint64_t FullBlockSize = Block->GetFullSize();
	uint64_t SizeWithHeader = Size + sizeof(BlockHeader);
	if (Size >= MIN_ALLOCATION_SIZE && BlockSize - SizeWithHeader >= MIN_ALLOCATION_SIZE)
	{
		BlockFreeHeader* BlockNextBlock = GetNextBlock(Block);
		void* AlignedBlock = bit::AlignPtr(bit::OffsetPtr(Block, SizeWithHeader), Alignment);
		BlockFreeHeader* RemainingBlock = bit::OffsetPtr<BlockFreeHeader>(AlignedBlock, -(intptr_t)sizeof(BlockHeader));
		uint64_t RemainingBlockSize = (uint64_t)bit::PtrDiff(BlockNextBlock, AlignedBlock);
		uint64_t NewBlockSize = (uint64_t)bit::PtrDiff(GetPointerFromBlockHeader(Block), RemainingBlock);
		RemainingBlock->Reset(RemainingBlockSize);
		RemainingBlock->SetUsed();
		RemainingBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Block);
		BlockFreeHeader* NextBlock = GetNextBlock(RemainingBlock);
		NextBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(RemainingBlock);
		Block->SetSize(NewBlockSize);
		BIT_ASSERT(FullBlockSize == Block->GetFullSize() + RemainingBlock->GetFullSize());
		if (RemainingBlock->GetSize() > Size)
		{
			BlockFreeHeader* NextRemainingBlock = Split(RemainingBlock, Size);
			if (RemainingBlock != NextRemainingBlock)
			{
				InsertBlock(NextRemainingBlock, Mapping(NextRemainingBlock->GetSize()));
			}
		}
		return RemainingBlock;
	}
	return Block;
}

void bit::MediumSizeAllocator::InsertBlock(BlockFreeHeader* Block, BlockMap Map)
{
	BIT_ASSERT(!Block->IsLastPhysicalBlock());
	BlockFreeHeader* Head = FreeBlocks[Map.FL][Map.SL];
	Block->NextFree = Head;
	Block->PrevFree = nullptr;
	Block->SetFree();
	if (Head != nullptr) Head->PrevFree = Block;
	FreeBlocks[Map.FL][Map.SL] = Block;
	FLBitmap |= Pow2(Map.FL);
	SLBitmap[Map.FL] |= Pow2(Map.SL);
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::MergeBlocks(BlockFreeHeader* Left, BlockFreeHeader* Right)
{
	Left->SetSize(Left->GetSize() + Right->GetFullSize());
	BlockFreeHeader* NextBlock = GetNextBlock(Left);
	NextBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Left);
	Right->Reset(0);
	return Left;
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::GetNextBlock(BlockFreeHeader* Block) const
{
	return bit::OffsetPtr<BlockFreeHeader>(Block, Block->GetSize() + sizeof(BlockHeader));
}

size_t bit::MediumSizeAllocator::AdjustSize(size_t Size)
{
	return sizeof(BlockHeader) + Size;
}

void bit::MediumSizeAllocator::FillBlock(BlockHeader* Block, uint8_t Value) const
{
	/* For debugging purpose */
	bit::Memset(GetPointerFromBlockHeader(Block), Value, Block->GetSize());
}
