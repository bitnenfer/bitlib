#include <bit/tlsf_allocator.h>
#include <bit/os.h>
#include <bit/scope_lock.h>

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
	bit::TScopedLock<CMutex> Lock(&Mutex);
	if (FLBitmap == 0) CreateAndInsertMemoryPool(Size); // No available blocks in the pool.

	uint32_t AlignedSize = (uint32_t)bit::AlignUint(Size, alignof(CBlockHeader));
	CBlockMap Map = Mapping(AlignedSize);
	CBlockFree* Block = FindSuitableBlock(AlignedSize, Map);
	if (Block == nullptr)
	{
		CreateAndInsertMemoryPool(Size); // No available blocks in the pool.
		Block = FindSuitableBlock(AlignedSize, Map);
	}
	if (Block != nullptr)
	{
		RemoveBlock(Block, Map);
		if (Block->Header.GetSize() > AlignedSize)
		{
			CBlockFree* RemainingBlock = Split(Block, AlignedSize);
			if (RemainingBlock != Block)
			{
				InsertBlock(RemainingBlock, Mapping(RemainingBlock->Header.GetSize()));
			}
		}
		AllocatedAmount += Block->Header.GetFullSize();
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
		bit::TScopedLock<CMutex> Lock(&Mutex);
		CBlockFree* FreeBlock = GetBlockHeaderFromPointer(Pointer);
		AllocatedAmount -= FreeBlock->Header.GetFullSize();
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
	bit::TScopedLock<CMutex> Lock(&Mutex);
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
	Block->Header.Reset(NewMemoryPool->UsableSize - sizeof(CBlockFree) - sizeof(CBlockHeader));
	InsertBlock(Block, Mapping(Block->Header.GetSize()));

	CBlockFree* EndOfBlock = GetNextBlock(Block);
	EndOfBlock->Header.Reset(0);
	EndOfBlock->Header.SetLastPhysicalBlock();
	EndOfBlock->Header.PrevPhysicalBlock = reinterpret_cast<CBlockHeader*>(Block);
	EndOfBlock->NextFree = nullptr;
	EndOfBlock->PrevFree = nullptr;
	
	NewMemoryPool->Next = MemoryPoolList;
	MemoryPoolList = NewMemoryPool;
}

bit::CTLSFAllocator::CMemoryPool* bit::CTLSFAllocator::CreateMemoryPool(size_t PoolSize)
{
	size_t AlignedSize = bit::RoundUp(bit::AlignUint(PoolSize + sizeof(CMemoryPool), alignof(CBlockHeader)), bit::GetOSAllocationGranularity() * 2);
	CVirtualAddressSpace VirtualAddress = {};
	if (VirtualReserveSpace(nullptr, AlignedSize, VirtualAddress))
	{
		CMemoryPool* Pool = reinterpret_cast<CMemoryPool*>(VirtualAddress.CommitAll());
		Pool->Next = nullptr;
		Pool->BaseAddress = bit::AlignPtr(bit::OffsetPtr(Pool, sizeof(CMemoryPool)), alignof(CBlockHeader));
		Pool->UsableSize = (uint32_t)bit::PtrDiff(Pool->BaseAddress, VirtualAddress.GetEndAddress());
		Pool->VirtualMemory = bit::Move(VirtualAddress);
		return Pool;
	}
	return nullptr;
}

bit::CTLSFAllocator::CBlockMap bit::CTLSFAllocator::Mapping(size_t Size) const
{
	// First level index is the last set bit
	uint64_t FL = bit::BitScanReverse64(Size);
	// Second level index is the next rightmost SLI bits
	// For example if SLI is 4, then 460 would result in 
	// 0000000111001100
	//       / |..|	
	//    FL    SL
	// FL = 8, SL = 12
	uint64_t SL = (Size >> (FL - SLI)) ^ (SL_COUNT);
	return { FL, SL };
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::GetBlockHeaderFromPointer(void* Block) const
{
	return bit::OffsetPtr<CBlockFree>(Block, -(int64_t)sizeof(CBlockHeader));
}

void* bit::CTLSFAllocator::GetPointerFromBlockHeader(CBlockFree* Block) const
{
	return bit::OffsetPtr(Block, sizeof(CBlockHeader));
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::Merge(CBlockFree* Block)
{
	CBlockFree* Next = GetNextBlock(Block);
	if (Next->Header.GetSize() > 0 && Next->Header.IsFree())
	{
		CBlockMap Map = Mapping(Next->Header.GetSize());
		RemoveBlock(Next, Map);
		return MergeBlocks(Block, Next);
	}

	CBlockFree* Prev = reinterpret_cast<CBlockFree*>(Block->Header.PrevPhysicalBlock);
	if (Prev != nullptr && Prev->Header.IsFree())
	{
		CBlockMap Map = Mapping(Prev->Header.GetSize());
		RemoveBlock(Prev, Map);
		return MergeBlocks(Prev, Block);
	}

	return Block;
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::FindSuitableBlock(size_t Size, CBlockMap& Map)
{
	uint64_t FL = Map.FL;
	uint64_t SL = Map.SL;
	uint64_t SLMapping = SLBitmap[FL] & (~0 << SL);
	if (SLMapping == 0)
	{
		uint64_t FLMapping = FLBitmap & (~0 << (FL + 1));
		if (FLMapping == 0)
		{
			// OOM - We need to allocate a new pool
			return nullptr;
		}
		FL = bit::BitScanForward(FLMapping);
		SLMapping = SLBitmap[FL];
	}
	SL = bit::BitScanForward(SLMapping);
	CBlockFree* Block = FreeBlocks[FL][SL];
	Map.FL = FL;
	Map.SL = SL;
	BIT_ASSERT(Block != nullptr);
	return Block;
}

void bit::CTLSFAllocator::RemoveBlock(CBlockFree* Block, CBlockMap Map)
{
	Block->Header.SetUsed();
	CBlockFree* Prev = Block->PrevFree;
	CBlockFree* Next = Block->NextFree;
	if (Prev != nullptr) Prev->NextFree = Next;
	if (Next != nullptr) Next->PrevFree = Prev;
	Block->PrevFree = nullptr;
	Block->NextFree = nullptr;

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
	if (Size > sizeof(CBlockHeader) && BlockSize - Size > sizeof(CBlockHeader))
	{
		CBlockFree* RemainingBlock = bit::OffsetPtr<CBlockFree>(Block, Size + sizeof(CBlockHeader));
		RemainingBlock->Header.Reset(BlockSize - Size - sizeof(CBlockHeader));
		RemainingBlock->Header.SetUsed();
		RemainingBlock->Header.PrevPhysicalBlock = reinterpret_cast<CBlockHeader*>(Block);
		CBlockFree* NextBlock = GetNextBlock(RemainingBlock);
		NextBlock->Header.PrevPhysicalBlock = reinterpret_cast<CBlockHeader*>(RemainingBlock);
		Block->Header.SetSize(Size);
		BIT_ASSERT(BlockSize + sizeof(CBlockHeader) == Block->Header.GetFullSize() + RemainingBlock->Header.GetFullSize());
		return RemainingBlock;
	}
	return Block;
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
	Left->Header.SetSize(Left->Header.GetSize() + Right->Header.GetFullSize());
	CBlockFree* NextBlock = GetNextBlock(Left);
	NextBlock->Header.PrevPhysicalBlock = reinterpret_cast<CBlockHeader*>(Left);
	Right->Header.Reset(0);
	return Left;
}

bit::CTLSFAllocator::CBlockFree* bit::CTLSFAllocator::GetNextBlock(CBlockFree* Block) const
{
	return bit::OffsetPtr<CBlockFree>(Block, Block->Header.GetSize() + sizeof(CBlockHeader));
}

size_t bit::CTLSFAllocator::AdjustSize(size_t Size)
{
	return sizeof(CBlockHeader) + Size;
}
