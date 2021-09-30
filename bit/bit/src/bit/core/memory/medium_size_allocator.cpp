#include <bit/core/memory/medium_size_allocator.h>
#include <bit/core/memory.h>
#include <bit/utility/scope_lock.h>
#include <bit/core/os/debug.h>
#include <bit/core/os/os.h>

#define BIT_ENABLE_BLOCK_MARKING 0

bit::MediumSizeAllocator::MediumSizeAllocator(size_t InitialPoolSize, const char* Name) :
	IAllocator(Name),
	FLBitmap(0),
	//MemoryPoolList(nullptr),
	//MemoryPoolCount(0),
	VirtualMemoryOffset(0),
	UsedSpaceInBytes(0),
	AvailableSpaceInBytes(0)
{
	bit::Memset(SLBitmap, 0, sizeof(SLBitmap));
	bit::Memset(FreeBlocks, 0, sizeof(FreeBlocks));

	/*if (InitialPoolSize > 0)
	{
		AddNewPool(InitialPoolSize);
	}*/

	if (!VirtualReserveSpace(VirtualRandomAddress(), ADDRESS_SPACE_SIZE, Memory))
	{
		BIT_PANIC();
	}
}

bit::MediumSizeAllocator::MediumSizeAllocator(const char* Name) :
	IAllocator(Name),
	FLBitmap(0),
	//MemoryPoolList(nullptr),
	///MemoryPoolCount(0),
	VirtualMemoryOffset(0),
	UsedSpaceInBytes(0),
	AvailableSpaceInBytes(0)
{
	bit::Memset(SLBitmap, 0, sizeof(SLBitmap));
	bit::Memset(FreeBlocks, 0, sizeof(FreeBlocks));
	
	if (!VirtualReserveSpace(VirtualRandomAddress(), ADDRESS_SPACE_SIZE, Memory))
	{
		BIT_PANIC();
	}
}

bit::MediumSizeAllocator::~MediumSizeAllocator()
{
	/*for (MemoryPool* Pool = MemoryPoolList; Pool != nullptr; )
	{
		MemoryPool* Next = Pool->Next;
		VirtualReleaseSpace(Pool->Memory);
		Pool = Next;
	}*/
	VirtualReleaseSpace(Memory);
}

void* bit::MediumSizeAllocator::Allocate(size_t Size, size_t Alignment)
{
	SizeType_t AdjustedSize = (SizeType_t)bit::Max((SizeType_t)Size, MIN_ALLOCATION_SIZE);
	//if (FLBitmap == 0) AddNewPool(AdjustedSize); // No available blocks in the pool.
	if (FLBitmap == 0) AllocateVirtualMemory(AdjustedSize);
	
	if (Alignment > alignof(BlockHeader))
	{
		return AllocateAligned(AdjustedSize, (SizeType_t)Alignment);
	}

	SizeType_t AlignedSize = (SizeType_t)bit::AlignUint(AdjustedSize, alignof(BlockHeader));
	BlockMap Map = Mapping(AlignedSize);
	BlockFreeHeader* Block = FindSuitableBlock(AlignedSize, Map);
	if (Block == nullptr)
	{
		//AddNewPool(Size); // No available blocks in the pool.
		AllocateVirtualMemory(Size);
		Block = FindSuitableBlock(AlignedSize, Map);
	}
	if (Block != nullptr)
	{
		SizeType_t BlockSize = Block->GetSize();
		SizeType_t BlockFullSize = Block->GetFullSize();
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
		/*if (!ReleaseUnusedMemoryPool(MergedBlock))
		{
			InsertBlock(MergedBlock, Mapping(MergedBlock->GetSize()));
		}*/
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

bit::MemoryUsageInfo bit::MediumSizeAllocator::GetMemoryUsageInfo()
{
	MemoryUsageInfo Usage = {};
	Usage.CommittedBytes = Memory.GetCommittedSize();
	Usage.ReservedBytes = Memory.GetReservedSize();
	/*for (MemoryPool* Pool = MemoryPoolList; Pool != nullptr; Pool = Pool->Next)
	{
		Usage.CommittedBytes += Pool->Memory.GetCommittedSize();
		Usage.ReservedBytes += Pool->Memory.GetRegionSize();
	}*/
	Usage.AllocatedBytes = UsedSpaceInBytes;
	return Usage;
}

bool bit::MediumSizeAllocator::CanAllocate(size_t Size, size_t Alignment)
{
	size_t AlignedSize = bit::AlignUint(Size, bit::Max(Alignment, alignof(BlockHeader)));
	return AlignedSize >= MIN_ALLOCATION_SIZE && AlignedSize <= MAX_ALLOCATION_SIZE;
}

bool bit::MediumSizeAllocator::OwnsAllocation(const void* Ptr)
{
	/*for (MemoryPool* Pool = MemoryPoolList; Pool != nullptr; Pool = Pool->Next)
	{
		if (Pool->Memory.OwnsAddress(Ptr))
		{
			return true;
		}
	}*/
	return Memory.OwnsAddress(Ptr);
	//return false;
}

size_t bit::MediumSizeAllocator::Compact()
{
	size_t Total = 0;
	/*for (MemoryPool* Pool = MemoryPoolList; Pool != nullptr;)
	{
		MemoryPool* Next = Pool->Next;
		if (RemoveMemoryPoolFreeBlocks(Pool))
		{
			ReleaseMemoryPool(Pool);
		}
		Pool = Next;
	}*/
	return Total;
}



/*void bit::MediumSizeAllocator::ReleaseMemoryPool(MemoryPool* Pool)
{
	if (Pool != nullptr)
	{
		MemoryPool* Prev = Pool->Prev;
		MemoryPool* Next = Pool->Next;
		if (Prev != nullptr) Prev->Next = Next;
		if (Next != nullptr) Next->Prev = Prev;
		if (Prev == nullptr) MemoryPoolList = Next;
		Pool->Memory.GetCommittedSize();
		MemoryPoolCount -= 1;
		VirtualReleaseSpace(Pool->Memory);
	}
}

bool bit::MediumSizeAllocator::ReleaseUnusedMemoryPool(BlockFreeHeader* FreeBlock)
{
	if (FreeBlock->PrevPhysicalBlock->IsLastPhysicalBlock() &&
		GetNextBlock(FreeBlock)->IsLastPhysicalBlock())
	{
		MemoryPool* Pool = reinterpret_cast<MemoryPool*>(FreeBlock->PrevPhysicalBlock);
		BIT_ASSERT(Pool->BaseAddress == FreeBlock);
		ReleaseMemoryPool(Pool);
		return true;
}
	return false;
}*/

void* bit::MediumSizeAllocator::AllocateAligned(SizeType_t Size, SizeType_t Alignment)
{
	SizeType_t AlignedSize = (SizeType_t)bit::AlignUint(Size, Alignment);
	BlockMap Map = Mapping(AlignedSize);
	BlockFreeHeader* Block = FindSuitableBlock(AlignedSize, Map);
	if (Block == nullptr)
	{
		//AddNewPool(Size); // No available blocks in the pool.
		AllocateVirtualMemory(Size);
		Block = FindSuitableBlock(AlignedSize, Map);
	}
	if (Block != nullptr)
	{
		SizeType_t BlockSize = Block->GetSize();
		SizeType_t BlockFullSize = Block->GetFullSize();
		RemoveBlock(Block, Map);
		if (Block->GetSize() > AlignedSize 
			&& (Block->GetSize() - AlignedSize) > MIN_ALLOCATION_SIZE && 
			bit::IsAddressAligned(GetPointerFromBlockHeader(Block), Alignment))
		{
			BlockFreeHeader* AlignedBlock = SplitAligned(Block, Size, Alignment);
			if (AlignedBlock != Block)
			{
				InsertBlock(Block, Mapping(Block->GetSize()));
			}
			Block = AlignedBlock;
		}
		UsedSpaceInBytes += Block->GetSize();
	#if BIT_ENABLE_BLOCK_MARKING
		FillBlock(Block, 0xAA);
	#endif
		return GetPointerFromBlockHeader(Block);
	}
	return nullptr; /* Out of memory */
}

/*bool bit::MediumSizeAllocator::IsPoolReleasable(MemoryPool* Pool)
{
	BlockFreeHeader* Block = reinterpret_cast<BlockFreeHeader*>(Pool->BaseAddress);
	while (Block != nullptr)
	{
		if (!Block->IsLastPhysicalBlock() && !Block->IsFree())
		{
			return false;
		}
		if (Block->GetSize() == 0 && 
			Block->IsLastPhysicalBlock()) break; // We reached the end of the pool
		Block = GetNextBlock(Block);
	}
	return true;
}

bool bit::MediumSizeAllocator::RemoveMemoryPoolFreeBlocks(MemoryPool* Pool)
{
	if (IsPoolReleasable(Pool))
	{
		BlockFreeHeader* Block = reinterpret_cast<BlockFreeHeader*>(Pool->BaseAddress);
		while (Block != nullptr)
		{
			if (Block->IsLastPhysicalBlock()) break;
			RemoveBlock(Block, Mapping(Block->GetSize()));
			Block = GetNextBlock(Block);
		}
		return true;
	}
	return false;
}*/

void bit::MediumSizeAllocator::AllocateVirtualMemory(size_t Size)
{
	size_t AlignedSize = bit::RoundUp(bit::AlignUint(Size, alignof(BlockHeader)), bit::GetOSPageSize() * 4);
	void* Pages = Memory.CommitPagesByOffset(VirtualMemoryOffset, AlignedSize);
	if (Memory.CommitPagesByOffset(VirtualMemoryOffset, AlignedSize) != nullptr)
	{
		VirtualMemoryOffset += AlignedSize;

		BlockFreeHeader* Block = reinterpret_cast<BlockFreeHeader*>(Pages);
		Block->Reset((SizeType_t)AlignedSize - sizeof(BlockFreeHeader) - sizeof(BlockHeader));
		Block->PrevPhysicalBlock = nullptr;
		InsertBlock(Block, Mapping(Block->GetSize()));

		BlockFreeHeader* EndOfBlock = GetNextBlock(Block);
		EndOfBlock->Reset(0);
		EndOfBlock->SetLastPhysicalBlock();
		EndOfBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Block);
		EndOfBlock->NextFree = nullptr;
		EndOfBlock->PrevFree = nullptr;

		AvailableSpaceInBytes += Block->GetSize();
	}
}

bool bit::MediumSizeAllocator::FreeVirtualMemory(BlockFreeHeader* FreeBlock)
{
	if (FreeBlock->PrevPhysicalBlock == nullptr && GetNextBlock(FreeBlock)->IsLastPhysicalBlock())
	{
		return Memory.DecommitPagesByAddress(FreeBlock, FreeBlock->GetFullSize() + sizeof(BlockFreeHeader));
	}
	return false;
}

/*
void bit::MediumSizeAllocator::AddNewPool(size_t PoolSize)
{
	size_t AlignedSize = bit::RoundUp(bit::AlignUint(PoolSize + sizeof(MemoryPool), alignof(BlockHeader)), bit::GetOSPageSize() * 4);
	VirtualAddressSpace VirtualAddress = {};
	if (VirtualReserveSpace(nullptr, AlignedSize, VirtualAddress))
	{
		MemoryPool* Pool = reinterpret_cast<MemoryPool*>(VirtualAddress.CommitAll());
		Pool->Next = nullptr;
		Pool->Prev = nullptr;
		Pool->BaseAddress = bit::AlignPtr(bit::OffsetPtr(VirtualAddress.GetBaseAddress(), sizeof(MemoryPool)), sizeof(BlockHeader));
		Pool->PoolSize = (SizeType_t)bit::PtrDiff(Pool->BaseAddress, VirtualAddress.GetEndAddress());
		Pool->Memory = bit::Move(VirtualAddress);
		Pool->BlockHead.Reset(0);
		Pool->BlockHead.SetLastPhysicalBlock();

		BlockFreeHeader* Block = reinterpret_cast<BlockFreeHeader*>(Pool->BaseAddress);
		Block->Reset((SizeType_t)Pool->PoolSize - sizeof(BlockFreeHeader) - sizeof(BlockHeader));
		Block->PrevPhysicalBlock = &Pool->BlockHead;
		InsertBlock(Block, Mapping(Block->GetSize()));

		BlockFreeHeader* EndOfBlock = GetNextBlock(Block);
		EndOfBlock->Reset(0);
		EndOfBlock->SetLastPhysicalBlock();
		EndOfBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Block);
		EndOfBlock->NextFree = nullptr;
		EndOfBlock->PrevFree = nullptr;
		
		if (MemoryPoolList != nullptr)
		{
			Pool->Next = MemoryPoolList;
			MemoryPoolList->Prev = Pool;
		}

		MemoryPoolList = Pool;
		AvailableSpaceInBytes += Block->GetSize();
		MemoryPoolCount += 1;
	}
}*/

bit::MediumSizeAllocator::BlockMap bit::MediumSizeAllocator::Mapping(size_t Size) const
{
	// First level index is the last set bit
	SizeType_t FL = bit::BitScanReverse((SizeType_t)Size);
	// Second level index is the next rightmost SLI bits
	// For example if SLI is 4, then 460 would result in 
	// 0000000111001100
	//       / |..|	
	//    FL    SL
	// FL = 8, SL = 12
	SizeType_t SL = ((SizeType_t)Size >> (FL - SLI)) ^ (SL_COUNT);
	FL -= COUNT_OFFSET - 1; // We need to adjust index so it maps to min alloc size range to max alloc size
	return { FL, SL };
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
	if (Prev != nullptr && Prev->IsFree())
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
	SizeType_t FL = Map.FL;
	SizeType_t SL = Map.SL;
	SizeType_t SLMapping = SLBitmap[FL] & (~(SizeType_t)0 << SL);
	if (SLMapping == 0)
	{
		SizeType_t FLMapping = FLBitmap & (~(SizeType_t)0 << (FL + 1));
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
	Map.FL = FL;
	Map.SL = SL;
	BIT_ASSERT(Block != nullptr);
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

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::Split(BlockFreeHeader* Block, SizeType_t Size)
{
	SizeType_t BlockSize = Block->GetSize();
	SizeType_t FullBlockSize = Block->GetFullSize();
	SizeType_t SizeWithHeader = Size + sizeof(BlockHeader);
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
		return RemainingBlock;
	}
	return Block;
}

bit::MediumSizeAllocator::BlockFreeHeader* bit::MediumSizeAllocator::SplitAligned(BlockFreeHeader* Block, SizeType_t Size, SizeType_t Alignment)
{
	SizeType_t BlockSize = Block->GetSize();
	SizeType_t FullBlockSize = Block->GetFullSize();
	SizeType_t SizeWithHeader = Size + sizeof(BlockHeader);
	if (Size >= MIN_ALLOCATION_SIZE && BlockSize - SizeWithHeader >= MIN_ALLOCATION_SIZE)
	{
		BlockFreeHeader* BlockNextBlock = GetNextBlock(Block);
		void* AlignedBlock = bit::AlignPtr(bit::OffsetPtr(Block, SizeWithHeader), Alignment);
		BlockFreeHeader* RemainingBlock = bit::OffsetPtr<BlockFreeHeader>(AlignedBlock, -(intptr_t)sizeof(BlockHeader));
		SizeType_t RemainingBlockSize = (SizeType_t)bit::PtrDiff(BlockNextBlock, AlignedBlock);
		SizeType_t NewBlockSize = (SizeType_t)bit::PtrDiff(GetPointerFromBlockHeader(Block), RemainingBlock);
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
