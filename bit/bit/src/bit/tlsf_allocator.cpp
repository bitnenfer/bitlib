#include <bit/tlsf_allocator.h>
#include <bit/os.h>

bit::CTLSFAllocator::CTLSFAllocator(size_t InitialPoolSize, const char* Name) :
	IAllocator(Name),
	FLBitmap(0),
	MemoryPoolList(nullptr),
	AllocatedAmount(0)
{
	bit::Memset(SLBitmap, 0, sizeof(SLBitmap));
	bit::Memset(FreeBlocks, 0, sizeof(FreeBlocks));

	if (InitialPoolSize > 0)
	{
		CreateAndInsertMemoryPool(InitialPoolSize);
	}
}

bit::CTLSFAllocator::CTLSFAllocator(const char* Name) :
	IAllocator(Name),
	FLBitmap(0),
	MemoryPoolList(nullptr),
	AllocatedAmount(0)
{
	bit::Memset(SLBitmap, 0, sizeof(SLBitmap));
	bit::Memset(FreeBlocks, 0, sizeof(FreeBlocks));
}

bit::CTLSFAllocator::~CTLSFAllocator()
{
	CMemoryPool* Block = MemoryPoolList;
	while (Block != nullptr)
	{
		CMemoryPool* Next = Block->Next;
		VirtualReleaseSpace(Block->VirtualMemory);
		Block = Next;
	}
}

void* bit::CTLSFAllocator::Allocate(size_t Size, size_t Alignment)
{
	uint32_t AlignedSize = (uint32_t)bit::AlignUint(Size, alignof(CBlockHeader));
	if (CBlockFree* Block = FindSuitableBlock(AlignedSize, Mapping(AlignedSize)))
	{
		RemoveBlock(Block);
		if (Block->Header.GetSize() > AlignedSize)
		{
			CBlockFree* RemainingBlock = Split(Block, AlignedSize);
			InsertBlock(RemainingBlock, Mapping(RemainingBlock->Header.GetSize()));
		}
		RemoveBlock(Block);
		AllocatedAmount += Block->Header.GetSize();
		return GetPointerFromBlockHeader(Block);
	}
	return nullptr; /* Out of memory */
}

void* bit::CTLSFAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr) return Allocate(Size, Alignment);
	size_t BlockSize = GetSize(Pointer);
	if (BlockSize == Size) return Pointer;
	void* NewBlock = Allocate(Size, Alignment);
	bit::Memcpy(NewBlock, Pointer, bit::Min(BlockSize, Size));
	Free(Pointer);
	return NewBlock;
}

void bit::CTLSFAllocator::Free(void* Pointer)
{
	if (Pointer != nullptr)
	{
		CBlockFree* FreeBlock = GetBlockHeaderFromPointer(Pointer);
		AllocatedAmount -= FreeBlock->Header.GetSize();
		CBlockFree* MergedBlock = Merge(FreeBlock);
		InsertBlock(MergedBlock, Mapping(MergedBlock->Header.GetSize()));
	}
}

size_t bit::CTLSFAllocator::GetSize(void* Pointer)
{
	if (Pointer != nullptr)
	{
		return GetBlockHeaderFromPointer(Pointer)->Header.GetSize();
	}
	return 0;
}

bit::CMemoryUsageInfo bit::CTLSFAllocator::GetMemoryUsageInfo()
{
	CMemoryUsageInfo Usage = {};
	CMemoryPool* Pool = MemoryPoolList;
	while (Pool != nullptr)
	{
		Usage.CommittedBytes += Pool->VirtualMemory.GetCommittedSize();
		Usage.ReservedBytes += Pool->VirtualMemory.GetRegionSize();
		Pool = Pool->Next;
	}
	Usage.AllocatedBytes = AllocatedAmount;
	return Usage;
}

size_t bit::CTLSFAllocator::TrimMemory()
{
	size_t Total = 0;
	CMemoryPool* Pool = MemoryPoolList;
	CMemoryPool* Prev = nullptr;
	while (Pool != nullptr)
	{
		CMemoryPool* Next = Pool->Next;
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

bool bit::CTLSFAllocator::IsPoolReleasable(CMemoryPool* Pool)
{
	CBlockFree* Block = reinterpret_cast<CBlockFree*>(Pool->BaseAddress);
	while (Block != nullptr)
	{
		if (!Block->Header.IsLastPhysicalBlock() && !Block->Header.IsFree())
		{
			return false;
		}
		if (Block->Header.GetSize() == 0 && 
			Block->Header.IsLastPhysicalBlock()) break; // We reached the end of the pool
		Block = GetNextBlock(Block);
	}
	return true;
}

void bit::CTLSFAllocator::CreateAndInsertMemoryPool(size_t PoolSize)
{
	InsertMemoryPool(CreateMemoryPool(PoolSize));
}

void bit::CTLSFAllocator::InsertMemoryPool(CMemoryPool* NewMemoryPool)
{
	CBlockFree* Block = reinterpret_cast<CBlockFree*>(NewMemoryPool->BaseAddress);
	Block->Header.Reset(NewMemoryPool->UsableSize - sizeof(CBlockFree));
	InsertBlock(Block, Mapping(Block->Header.GetSize()));

	CBlockFree* EndOfBlock = bit::ForwardPtr<CBlockFree>(Block, Block->Header.GetSize() + sizeof(CBlockHeader));
	EndOfBlock->Header.Reset(0);
	EndOfBlock->Header.SetLastPhysicalBlock();
	EndOfBlock->Header.PrevPhysicalBlock = reinterpret_cast<CBlockHeader*>(Block);

	CBlockFree* NextMaybe = GetNextBlock(Block);
	
	NewMemoryPool->Next = MemoryPoolList;
	MemoryPoolList = NewMemoryPool;
}

bit::CTLSFAllocator::CMemoryPool* bit::CTLSFAllocator::CreateMemoryPool(size_t PoolSize)
{
	size_t AlignedSize = bit::RoundUp(bit::AlignUint(PoolSize + sizeof(CMemoryPool), alignof(CBlockHeader)), bit::GetOSAllocationGranularity());
	CVirtualAddressSpace VirtualAddress = {};
	if (VirtualReserveSpace(nullptr, AlignedSize, VirtualAddress))
	{
		CMemoryPool* Pool = reinterpret_cast<CMemoryPool*>(VirtualAddress.CommitAll());
		Pool->Next = nullptr;
		Pool->UsableSize = (uint32_t)VirtualAddress.GetRegionSize() - sizeof(CMemoryPool);
		Pool->BaseAddress = bit::ForwardPtr(Pool, sizeof(CMemoryPool));
		Pool->VirtualMemory = bit::Move(VirtualAddress);
		return Pool;
	}
	return nullptr;
}

bit::CTLSFAllocator::CBlockMap bit::CTLSFAllocator::Mapping(size_t Size) const
{
	// First level index is the last set bit
	size_t FL = bit::Log2(Size);
	// Second level index is the next rightmost SLI bits
	// For example if SLI is 4, then 460 would result in 
	// 0000000111001100
	//       / |..|	
	//    FL    SL
	// FL = 8, SL = 12
	size_t SL = (Size >> (FL - SLI)) ^ (SL_COUNT);
	return { FL, SL };
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::GetBlockHeaderFromPointer(void* Block) const
{
	return bit::BackwardPtr<CBlockFree>(Block, sizeof(CBlockHeader));
}

void* bit::CTLSFAllocator::GetPointerFromBlockHeader(CBlockFree* Block) const
{
	return bit::ForwardPtr(Block, sizeof(CBlockHeader));
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::Merge(CBlockFree* Block)
{
	CBlockFree* Next = GetNextBlock(Block);
	if (Next->Header.GetSize() > 0 && Next->Header.IsFree())
	{
		RemoveBlock(Next);
		return MergeBlocks(Block, Next);
	}

	CBlockFree* Prev = reinterpret_cast<CBlockFree*>(Block->Header.PrevPhysicalBlock);
	if (Prev != nullptr && Prev->Header.IsFree())
	{
		RemoveBlock(Prev);
		return MergeBlocks(Prev, Block);
	}

	return Block;
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::FindSuitableBlock(size_t Size, CBlockMap Map)
{
	bool bTryAgain = true;
RetryAllocation:
	CBlockFree* Block = FreeBlocks[Map.FL][Map.SL];
	if (Block != nullptr)
	{
		return Block;
	}
	size_t NextFreeFL = bit::Log2(FLBitmap);
	if ((1ULL << NextFreeFL) >= Size)
	{
		size_t NextFreeSL = bit::Log2(SLBitmap[NextFreeFL]);
		CBlockFree* FreeBlock = FreeBlocks[NextFreeFL][NextFreeSL];
		if (FreeBlock != nullptr && FreeBlock->Header.GetSize() >= Size)
		{
			return FreeBlock;
		}
	}
	if (bTryAgain)
	{
		CreateAndInsertMemoryPool(Size);
		goto RetryAllocation;
		bTryAgain = false;
	}

	return nullptr;
}

void bit::CTLSFAllocator::RemoveBlock(CBlockFree* Block)
{
	Block->Header.SetUsed();
	CBlockFree* Prev = Block->PrevFree;
	CBlockFree* Next = Block->NextFree;
	if (Prev != nullptr) Prev->NextFree = Next;
	if (Next != nullptr) Next->PrevFree = Prev;
	Block->PrevFree = nullptr;
	Block->NextFree = nullptr;
	CBlockMap Map = Mapping(Block->Header.GetSize());
	if (FreeBlocks[Map.FL][Map.SL] == Block)
	{
		FreeBlocks[Map.FL][Map.SL] = Next;
		if (Next == nullptr)
		{
			SLBitmap[Map.FL] &= ~(1 << Map.SL);
			if (SLBitmap[Map.FL] == 0)
			{
				FLBitmap &= ~(1 << Map.FL);
			}
		}
	}
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::Split(CBlockFree* Block, uint32_t Size)
{
	uint32_t BlockSize = Block->Header.GetSize();
	uint32_t RemainingSize = BlockSize - Size;
	CBlockFree* RemainingBlock = bit::ForwardPtr<CBlockFree>(Block, Size + sizeof(CBlockHeader));
	RemainingBlock->Header.Reset(RemainingSize);
	RemainingBlock->Header.PrevPhysicalBlock = reinterpret_cast<CBlockHeader*>(Block);
	Block->Header.SetSize(Size);
	return RemainingBlock;
}

void bit::CTLSFAllocator::InsertBlock(CBlockFree* Block, CBlockMap Map)
{
	CBlockFree* Head = FreeBlocks[Map.FL][Map.SL];
	Block->NextFree = Head;
	Block->PrevFree = nullptr;
	Block->Header.SetFree();
	if (Head != nullptr) Head->PrevFree = Block;
	FreeBlocks[Map.FL][Map.SL] = Block;
	FLBitmap |= (1 << Map.FL);
	SLBitmap[Map.FL] |= (1 << Map.SL);
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::MergeBlocks(CBlockFree* Left, CBlockFree* Right)
{
	Left->Header.SetSize(Left->Header.GetSize() + Right->Header.GetSize());
	CBlockFree* NextBlock = GetNextBlock(Left);
	NextBlock->Header.PrevPhysicalBlock = reinterpret_cast<CBlockHeader*>(Left);
	Right->Header.Reset(0);
	return Left;
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::GetNextBlock(CBlockFree* Block) const
{
	return bit::ForwardPtr<CBlockFree>(Block, Block->Header.GetSize() + sizeof(CBlockHeader));
}
