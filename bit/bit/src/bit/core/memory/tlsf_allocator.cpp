#include <bit/core/memory/tlsf_allocator.h>
#include <bit/core/os/scope_lock.h>
#include <bit/core/os/debug.h>
#include <bit/core/os/os.h>

#define BIT_ENABLE_BLOCK_MARKING 0

bit::TLSFAllocator::TLSFAllocator(size_t InitialPoolSize, const char* Name) :
	IAllocator(Name),
	FLBitmap(0),
	MemoryPoolList(nullptr),
	MemoryPoolCount(0),
	UsedSpaceInBytes(0),
	AvailableSpaceInBytes(0)
{
	bit::Memset(SLBitmap, 0, sizeof(SLBitmap));
	bit::Memset(FreeBlocks, 0, sizeof(FreeBlocks));

	if (InitialPoolSize > 0)
	{
		CreateAndInsertMemoryPool(InitialPoolSize);
	}
}

bit::TLSFAllocator::TLSFAllocator(const char* Name) :
	IAllocator(Name),
	FLBitmap(0),
	MemoryPoolList(nullptr),
	MemoryPoolCount(0),
	UsedSpaceInBytes(0),
	AvailableSpaceInBytes(0)
{
	bit::Memset(SLBitmap, 0, sizeof(SLBitmap));
	bit::Memset(FreeBlocks, 0, sizeof(FreeBlocks));
}

bit::TLSFAllocator::~TLSFAllocator()
{
	MemoryPool* Block = MemoryPoolList;
	while (Block != nullptr)
	{
		MemoryPool* Next = Block->Next;
		VirtualReleaseSpace(Block->VirtualMemory);
		Block = Next;
	}
}

void* bit::TLSFAllocator::Allocate(size_t Size, size_t Alignment)
{
	bit::ScopedLock<Mutex> Lock(&AccessLock);
	if (FLBitmap == 0) CreateAndInsertMemoryPool(Size); // No available blocks in the pool.

	TLSFSizeType_t AdjustedSize = (TLSFSizeType_t)bit::Max((TLSFSizeType_t)Size, MIN_ALLOC_SIZE);
	TLSFSizeType_t AlignedSize = (TLSFSizeType_t)bit::AlignUint(AdjustedSize, alignof(BlockHeader));
	BlockMap Map = Mapping(AlignedSize);
	BlockFreeHeader* Block = FindSuitableBlock(AlignedSize, Map);
	if (Block == nullptr)
	{
		CreateAndInsertMemoryPool(Size); // No available blocks in the pool.
		Block = FindSuitableBlock(AlignedSize, Map);
	}
	if (Block != nullptr)
	{
		TLSFSizeType_t BlockSize = Block->GetSize();
		TLSFSizeType_t BlockFullSize = Block->GetFullSize();
		RemoveBlock(Block, Map);
		if (Block->GetSize() > AlignedSize && (Block->GetSize() - AlignedSize) > MIN_ALLOC_SIZE)
		{
			BlockFreeHeader* RemainingBlock = Split(Block, AlignedSize);
			if (RemainingBlock != Block)
			{
				InsertBlock(RemainingBlock, Mapping(RemainingBlock->GetSize()));
			}
		}
		UsedSpaceInBytes += Block->GetFullSize();
	#if BIT_ENABLE_BLOCK_MARKING
		FillBlock(Block, 0xAA);
	#endif
		return GetPointerFromBlockHeader(Block);
	}
	return nullptr; /* Out of memory */
}

void* bit::TLSFAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr) return Allocate(Size, Alignment);
	size_t BlockSize = GetSize(Pointer);
	if (BlockSize == Size) return Pointer;
	void* NewBlock = Allocate(Size, Alignment);
	bit::Memcpy(NewBlock, Pointer, bit::Min(BlockSize, Size));
	Free(Pointer);
	return NewBlock;
}

void bit::TLSFAllocator::Free(void* Pointer)
{
	if (Pointer != nullptr)
	{
		bit::ScopedLock<Mutex> Lock(&AccessLock);
		BlockFreeHeader* FreeBlock = GetBlockHeaderFromPointer(Pointer);
	#if BIT_ENABLE_BLOCK_MARKING
		FillBlock(FreeBlock, 0xDD);
	#endif
		UsedSpaceInBytes -= FreeBlock->GetFullSize();
		BlockFreeHeader* MergedBlock = Merge(FreeBlock);
		InsertBlock(MergedBlock, Mapping(MergedBlock->GetSize()));
	}
}

size_t bit::TLSFAllocator::GetSize(void* Pointer)
{
	if (Pointer != nullptr)
	{
		return GetBlockHeaderFromPointer(Pointer)->GetSize();
	}
	return 0;
}

bit::MemoryUsageInfo bit::TLSFAllocator::GetMemoryUsageInfo()
{
	MemoryUsageInfo Usage = {};
	MemoryPool* Pool = MemoryPoolList;
	while (Pool != nullptr)
	{
		Usage.CommittedBytes += Pool->VirtualMemory.GetCommittedSize();
		Usage.ReservedBytes += Pool->VirtualMemory.GetRegionSize();
		Pool = Pool->Next;
	}
	Usage.AllocatedBytes = UsedSpaceInBytes;
	return Usage;
}

size_t bit::TLSFAllocator::TrimMemory()
{
	bit::ScopedLock<Mutex> Lock(&AccessLock);
	size_t Total = 0;
	MemoryPool* Pool = MemoryPoolList;
	MemoryPool* Prev = nullptr;
	while (Pool != nullptr)
	{
		MemoryPool* Next = Pool->Next;
		if (IsPoolReleasable(Pool))
		{
			VirtualReleaseSpace(Pool->VirtualMemory);
			if (Prev != nullptr) Prev->Next = Next;
			else MemoryPoolList = nullptr;
			Prev = Next;
		}
		else
		{
			Prev = Pool;
		}
		Pool = Next;
	}
	return Total;
}

bool bit::TLSFAllocator::IsPoolReleasable(MemoryPool* Pool)
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

void bit::TLSFAllocator::CreateAndInsertMemoryPool(size_t PoolSize)
{
	InsertMemoryPool(CreateMemoryPool(PoolSize));
}

void bit::TLSFAllocator::InsertMemoryPool(MemoryPool* NewMemoryPool)
{
	BlockFreeHeader* Block = reinterpret_cast<BlockFreeHeader*>(NewMemoryPool->BaseAddress);
	Block->Reset(NewMemoryPool->UsableSize - sizeof(BlockFreeHeader) - sizeof(BlockHeader));
	InsertBlock(Block, Mapping(Block->GetSize()));

	BlockFreeHeader* EndOfBlock = GetNextBlock(Block);
	EndOfBlock->Reset(0);
	EndOfBlock->SetLastPhysicalBlock();
	EndOfBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Block);
	EndOfBlock->NextFree = nullptr;
	EndOfBlock->PrevFree = nullptr;
	
	NewMemoryPool->Next = MemoryPoolList;
	MemoryPoolList = NewMemoryPool;
	AvailableSpaceInBytes += Block->GetFullSize();
	MemoryPoolCount += 1;
}

bit::TLSFAllocator::MemoryPool* bit::TLSFAllocator::CreateMemoryPool(size_t PoolSize)
{
	size_t AlignedSize = bit::RoundUp(bit::AlignUint(PoolSize + sizeof(MemoryPool), alignof(BlockHeader)), bit::GetOSAllocationGranularity());
	VirtualAddressSpace VirtualAddress = {};
	if (VirtualReserveSpace(nullptr, AlignedSize, VirtualAddress))
	{
		MemoryPool* Pool = reinterpret_cast<MemoryPool*>(VirtualAddress.CommitAll());
		Pool->Next = nullptr;
		Pool->BaseAddress = bit::AlignPtr(bit::OffsetPtr(Pool, sizeof(MemoryPool)), alignof(BlockHeader));
		Pool->UsableSize = (TLSFSizeType_t)bit::PtrDiff(Pool->BaseAddress, VirtualAddress.GetEndAddress());
		Pool->VirtualMemory = bit::Move(VirtualAddress);
		return Pool;
	}
	return nullptr;
}

bit::TLSFAllocator::BlockMap bit::TLSFAllocator::Mapping(size_t Size) const
{
	// First level index is the last set bit
	TLSFSizeType_t FL = bit::BitScanReverse((TLSFSizeType_t)Size);
	// Second level index is the next rightmost SLI bits
	// For example if SLI is 4, then 460 would result in 
	// 0000000111001100
	//       / |..|	
	//    FL    SL
	// FL = 8, SL = 12
	TLSFSizeType_t SL = ((TLSFSizeType_t)Size >> (FL - SLI)) ^ (SL_COUNT);
	FL -= COUNT_OFFSET - 1; // We need to adjust index so it maps to min alloc size range to max alloc size
	return { FL, SL };
}

bit::TLSFAllocator::BlockFreeHeader* bit::TLSFAllocator::GetBlockHeaderFromPointer(void* Block) const
{
	return bit::OffsetPtr<BlockFreeHeader>(Block, -(intptr_t)sizeof(BlockHeader));
}

void* bit::TLSFAllocator::GetPointerFromBlockHeader(BlockHeader* Block) const
{
	return bit::OffsetPtr(Block, sizeof(BlockHeader));
}

bit::TLSFAllocator::BlockFreeHeader* bit::TLSFAllocator::Merge(BlockFreeHeader* Block)
{
	return MergeRecursive(Block);
}

bit::TLSFAllocator::BlockFreeHeader* bit::TLSFAllocator::MergeRecursive(BlockFreeHeader* Block)
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

bit::TLSFAllocator::BlockFreeHeader* bit::TLSFAllocator::FindSuitableBlock(size_t Size, BlockMap& Map)
{
	TLSFSizeType_t FL = Map.FL;
	TLSFSizeType_t SL = Map.SL;
	TLSFSizeType_t SLMapping = SLBitmap[FL] & (~0 << SL);
	if (SLMapping == 0)
	{
		TLSFSizeType_t FLMapping = FLBitmap & (~0 << (FL + 1));
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

void bit::TLSFAllocator::RemoveBlock(BlockFreeHeader* Block, BlockMap Map)
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

bit::TLSFAllocator::BlockFreeHeader* bit::TLSFAllocator::Split(BlockFreeHeader* Block, TLSFSizeType_t Size)
{
	TLSFSizeType_t BlockSize = Block->GetSize();
	TLSFSizeType_t FullBlockSize = Block->GetFullSize();
	TLSFSizeType_t SizeWithHeader = Size + sizeof(BlockHeader);
	if (Size >= MIN_ALLOC_SIZE && BlockSize - SizeWithHeader >= MIN_ALLOC_SIZE)
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

void bit::TLSFAllocator::InsertBlock(BlockFreeHeader* Block, BlockMap Map)
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

bit::TLSFAllocator::BlockFreeHeader* bit::TLSFAllocator::MergeBlocks(BlockFreeHeader* Left, BlockFreeHeader* Right)
{
	Left->SetSize(Left->GetSize() + Right->GetFullSize());
	BlockFreeHeader* NextBlock = GetNextBlock(Left);
	NextBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Left);
	Right->Reset(0);
	return Left;
}

bit::TLSFAllocator::BlockFreeHeader* bit::TLSFAllocator::GetNextBlock(BlockFreeHeader* Block) const
{
	return bit::OffsetPtr<BlockFreeHeader>(Block, Block->GetSize() + sizeof(BlockHeader));
}

size_t bit::TLSFAllocator::AdjustSize(size_t Size)
{
	return sizeof(BlockHeader) + Size;
}

void bit::TLSFAllocator::FillBlock(BlockHeader* Block, uint8_t Value) const
{
	/* For debugging purpose */
	bit::Memset(GetPointerFromBlockHeader(Block), Value, Block->GetSize());
}
