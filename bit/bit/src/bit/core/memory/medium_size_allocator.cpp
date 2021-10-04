#include <bit/core/memory/medium_size_allocator.h>
#include <bit/core/memory.h>
#include <bit/utility/scope_lock.h>
#include <bit/core/os/debug.h>
#include <bit/core/os/os.h>

#define BIT_ENABLE_BLOCK_MARKING 1

bit::MediumSizeAllocator::MediumSizeAllocator() :
	FLBitmap(0),
#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
	PagesInUse(nullptr),
#else
	MemoryPoolList(nullptr),
	MemoryPoolCount(0),
#endif
	UsedSpaceInBytes(0),
	AvailableSpaceInBytes(0)
{
#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
	if (!VirtualReserveSpace((void*)0x80000000ULL, ADDRESS_SPACE_SIZE, Memory))
	{
		BIT_PANIC();
	}
	VirtualMemoryBaseOffset = 0;
	VirtualMemoryBaseAddress = Memory.GetBaseAddress();
#endif

	bit::Memset(SLBitmap, 0, sizeof(SLBitmap));
	bit::Memset(FreeBlocks, 0, sizeof(FreeBlocks));
}

bit::MediumSizeAllocator::~MediumSizeAllocator()
{
#if !BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
	for (MemoryPool* Pool = MemoryPoolList; Pool != nullptr; )
	{
		MemoryPool* Next = Pool->Next;
		VirtualReleaseSpace(Pool->Memory);
		Pool = Next;
	}
#endif
}

void* bit::MediumSizeAllocator::Allocate(size_t Size, size_t Alignment)
{
	SizeType_t AdjustedSize = (SizeType_t)bit::Max((SizeType_t)Size, MIN_ALLOCATION_SIZE);
#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
	if (FLBitmap == 0) AllocateVirtualMemory(AdjustedSize);
#else
	if (FLBitmap == 0) AddNewPool(AdjustedSize); // No available blocks in the pool.
#endif

	if (Alignment > alignof(BlockHeader))
	{
		return AllocateAligned(AdjustedSize, (SizeType_t)Alignment);
	}

	SizeType_t AlignedSize = (SizeType_t)bit::AlignUint(AdjustedSize, alignof(BlockHeader));
	BlockMap Map = Mapping(AlignedSize);
	BlockFreeHeader* Block = FindSuitableBlock(AlignedSize, Map);
	if (Block == nullptr)
	{
#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
		AllocateVirtualMemory(Size);
#else
		AddNewPool(Size); // No available blocks in the pool.
	#endif
		Block = FindSuitableBlock(AlignedSize, Map);
	}
	if (Block != nullptr)
	{
		BIT_ASSERT(!Block->IsLastPhysicalBlock());
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

#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
		if (!FreeVirtualMemory(MergedBlock))
		{
			InsertBlock(MergedBlock, Mapping(MergedBlock->GetSize()));
		}
#else
		if (!ReleaseUnusedMemoryPool(MergedBlock))
		{
			InsertBlock(MergedBlock, Mapping(MergedBlock->GetSize()));
		}
#endif
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

#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
	AllocatorMemoryInfo VmUsage = {};
	Usage.CommittedBytes = Memory.GetCommittedSize();
	Usage.ReservedBytes = Memory.GetReservedSize();
#else
	for (MemoryPool* Pool = MemoryPoolList; Pool != nullptr; Pool = Pool->Next)
	{
		Usage.CommittedBytes += Pool->Memory.GetCommittedSize();
		Usage.ReservedBytes += Pool->Memory.GetReservedSize();
	}
#endif
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
#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
	return Memory.OwnsAddress(Ptr);
#else
	for (MemoryPool* Pool = MemoryPoolList; Pool != nullptr; Pool = Pool->Next)
	{
		if (Pool->Memory.OwnsAddress(Ptr))
		{
			return true;
		}
	}
	return false;
#endif
}

size_t bit::MediumSizeAllocator::Compact()
{
	size_t Total = 0;
#if !BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
	for (MemoryPool* Pool = MemoryPoolList; Pool != nullptr;)
	{
		MemoryPool* Next = Pool->Next;
		if (RemoveMemoryPoolFreeBlocks(Pool))
		{
			ReleaseMemoryPool(Pool);
		}
		Pool = Next;
	}
#endif
	return Total;
}

void* bit::MediumSizeAllocator::AllocateAligned(SizeType_t Size, SizeType_t Alignment)
{
	SizeType_t AlignedSize = (SizeType_t)bit::AlignUint(Size, Alignment);
	BlockMap Map = Mapping(AlignedSize);
	BlockFreeHeader* Block = FindSuitableBlock(AlignedSize, Map);
	if (Block == nullptr)
	{
	#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
		AllocateVirtualMemory(Size);
	#else
		AddNewPool(Size); // No available blocks in the pool.
	#endif
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

#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
void bit::MediumSizeAllocator::AllocateVirtualMemory(size_t Size)
{
	size_t PageSize = GetOSPageSize();
	size_t AlignedSize = bit::RoundUp(bit::AlignUint(Size + sizeof(BlockHeader) + sizeof(VirtualPage), alignof(VirtualPage)), PageSize);
	void* CommittedPages = Memory.CommitPagesByAddress(bit::OffsetPtr(VirtualMemoryBaseAddress, VirtualMemoryBaseOffset), AlignedSize);
	if (CommittedPages != nullptr)
	{
		VirtualMemoryBaseOffset += AlignedSize;
		VirtualPage* VPages = reinterpret_cast<VirtualPage*>(CommittedPages);
		VPages->Next = PagesInUse;
		VPages->Prev = nullptr;
		if (PagesInUse != nullptr) PagesInUse->Prev = VPages;
		PagesInUse = VPages;
		VPages->BlockHead.Reset(0);
		VPages->BlockHead.SetLastPhysicalBlock();
		VPages->PageSize = AlignedSize;

		void* Offset = bit::OffsetPtr(VPages, sizeof(VirtualPage));
		BlockFreeHeader* Block = reinterpret_cast<BlockFreeHeader*>(bit::AlignPtr(Offset, alignof(BlockHeader)));
		void* StartPtr = GetPointerFromBlockHeader(Block);
		size_t BlockDiff = bit::PtrDiff(StartPtr, bit::OffsetPtr(CommittedPages, AlignedSize));
		size_t BlockSize = BlockDiff - sizeof(BlockFreeHeader);
		Block->Reset(BlockSize - sizeof(BlockFreeHeader));
		Block->PrevPhysicalBlock = &VPages->BlockHead;

		BlockFreeHeader* EndOfBlock = GetNextBlock(Block);
		EndOfBlock->Reset(0);
		EndOfBlock->SetLastPhysicalBlock();
		EndOfBlock->PrevPhysicalBlock = reinterpret_cast<BlockHeader*>(Block);
		EndOfBlock->NextFree = nullptr;
		EndOfBlock->PrevFree = nullptr;

		if (Block->GetSize() > MAX_ALLOCATION_SIZE)
		{
			if (MAX_ALLOCATION_SIZE - Block->GetSize() >= MIN_ALLOCATION_SIZE + sizeof(BlockHeader))
			{
				BlockFreeHeader* Remaining = Split(Block, bit::Min(Size, MAX_ALLOCATION_SIZE));
				InsertBlock(Block, Mapping(Block->GetSize()));
				InsertBlock(Remaining, Mapping(Remaining->GetSize()));
			}
			else
			{
				BIT_PANIC();
			}
		}
		else
		{
			InsertBlock(Block, Mapping(Block->GetSize()));
		}

		BIT_ASSERT(Block->GetSize() < MAX_ALLOCATION_SIZE);

		AvailableSpaceInBytes += Block->GetSize();
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

		Memory.DecommitPagesByAddress(Page, Page->PageSize);
		return true;
	}
	return false;
}
#else
bool bit::MediumSizeAllocator::IsPoolReleasable(MemoryPool* Pool)
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
}

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
}

void bit::MediumSizeAllocator::ReleaseMemoryPool(MemoryPool* Pool)
{
	if (Pool != nullptr)
	{
		MemoryPool* Prev = Pool->Prev;
		MemoryPool* Next = Pool->Next;
		if (Prev != nullptr) Prev->Next = Next;
		if (Next != nullptr) Next->Prev = Prev;
		if (Prev == nullptr) MemoryPoolList = Next;
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
}
#endif

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
#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
	if (!Prev->IsLastPhysicalBlock() && Prev->IsFree())
	#else
	if (Prev != nullptr && Prev->IsFree())
#endif
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
