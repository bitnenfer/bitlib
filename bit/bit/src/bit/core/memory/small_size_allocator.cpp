#include <bit/core/memory/small_size_allocator.h>
#include <bit/core/memory.h>
#include <bit/core/os/debug.h>

#define BIT_DEBUG_MARK_BLOCKS 0

bit::SmallSizeAllocator::SmallSizeAllocator(size_t ReservedSpaceInBytes, const char* Name) :
	SmallSizeAllocator(Name)
{
	BIT_ASSERT((ReservedSpaceInBytes % NUM_OF_SIZES) == 0);

	if (!VirtualReserveSpace(VirtualRandomAddress(), ReservedSpaceInBytes, Memory))
	{
		BIT_PANIC();
	}

	size_t RangeSize = ReservedSpaceInBytes / NUM_OF_SIZES;
	void* BaseAddress = bit::AlignPtr(Memory.GetBaseAddress(), MIN_ALLOCATION_SIZE);

	for (size_t Index = 0; Index < NUM_OF_SIZES; ++Index)
	{
		size_t BlockSize = GetBlockSize(Index);
		Blocks[Index].Committed = 0;
		Blocks[Index].Start = bit::AlignPtr(BaseAddress, BlockSize);
		Blocks[Index].End = bit::OffsetPtr(BaseAddress, RangeSize);
		BaseAddress = Blocks[Index].End;
	}
}

bit::SmallSizeAllocator::SmallSizeAllocator(const char* Name) :
	IAllocator(Name),
	UsedSpaceInBytes(0)
{
	bit::Memset(Blocks, 0, sizeof(Blocks));
	bit::Memset(FreeLists, 0, sizeof(FreeLists));
}

void* bit::SmallSizeAllocator::Allocate(size_t Size, size_t Alignment)
{
	size_t AlignedSize = bit::AlignUint(Size, bit::Max(MIN_ALLOCATION_SIZE, Alignment));
	size_t BlockIndex = SelectBlockIndex(AlignedSize);
	FreeBlock* Block = PopFreeBlock(BlockIndex);
	if (Block != nullptr)
	{
		return Block;
	}
	return  AllocateFromVirtualBlock(BlockIndex, AlignedSize);
}

void* bit::SmallSizeAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr) return Allocate(Size, Alignment);
	size_t BlockSize = GetSize(Pointer);
	if (BlockSize == Size) return Pointer;
	void* NewBlock = Allocate(Size, Alignment);
	bit::Memcpy(NewBlock, Pointer, bit::Min(BlockSize, Size));
	Free(Pointer);
	return NewBlock;
}

void bit::SmallSizeAllocator::Free(void* Pointer)
{
	size_t BlockSize = GetSize(Pointer);
	if (BlockSize > 0)
	{
		size_t Index = SelectBlockIndex(BlockSize);
#if BIT_DEBUG_MARK_BLOCKS
		bit::Memset(Pointer, 0xDD, BlockSize);
#endif
		FreeBlock* Block = reinterpret_cast<FreeBlock*>(Pointer);
		Block->Next = FreeLists[Index];
		FreeLists[Index] = Block;
		UsedSpaceInBytes -= BlockSize;
		Blocks[Index].Allocated -= BlockSize;
		if (Blocks[Index].Allocated == 0)
		{
			Memory.DecommitPagesByAddress(Blocks[Index].Start, Blocks[Index].Committed);
			FreeLists[Index] = nullptr;
			Blocks[Index].Committed = 0;
		}
	}
}

size_t bit::SmallSizeAllocator::GetSize(void* Pointer)
{
	if (Memory.OwnsAddress(Pointer))
	{
		for (size_t Index = 0; Index < NUM_OF_SIZES; ++Index)
		{
			if (Blocks[Index].OwnsPointer(Pointer))
			{
				return GetBlockSize(Index);
			}
		}
	}
	return 0;
}

bit::MemoryUsageInfo bit::SmallSizeAllocator::GetMemoryUsageInfo()
{
	MemoryUsageInfo Usage = {};
	Usage.ReservedBytes = Memory.GetRegionSize();
	Usage.CommittedBytes = Memory.GetCommittedSize();
	Usage.AllocatedBytes = UsedSpaceInBytes;
	return Usage;
}

bool bit::SmallSizeAllocator::CanAllocate(size_t Size, size_t Alignment)
{
	return bit::AlignUint(Size, bit::Max(MIN_ALLOCATION_SIZE, Alignment)) <= MAX_ALLOCATION_SIZE;
}

bool bit::SmallSizeAllocator::OwnsAllocation(const void* Ptr)
{
	return Memory.OwnsAddress(Ptr);
}

void* bit::SmallSizeAllocator::AllocateFromVirtualBlock(size_t BlockIndex, size_t AlignedSize)
{
	size_t BlockSize = GetBlockSize(BlockIndex);
	size_t PageSizeAligned = bit::AlignUint(BlockSize, bit::GetOSPageSize());
	void* CurrentAddress = bit::OffsetPtr(Blocks[BlockIndex].Start, Blocks[BlockIndex].Committed);
	CurrentAddress = Memory.CommitPagesByAddress(CurrentAddress, PageSizeAligned);
	Blocks[BlockIndex].Committed += PageSizeAligned;
	size_t BlockCount = PageSizeAligned / BlockSize;
	// Fill up the free list with the new blocks
	for (size_t Index = 0; Index < BlockCount; ++Index)
	{
		FreeBlock* NewBlock = reinterpret_cast<FreeBlock*>(bit::OffsetPtr(CurrentAddress, Index * BlockSize));
		NewBlock->Next = FreeLists[BlockIndex];
		FreeLists[BlockIndex] = NewBlock;
	}
	// Return the last block
	return PopFreeBlock(BlockIndex);
}

size_t bit::SmallSizeAllocator::SelectBlockIndex(size_t Size)
{
	if (Size <= MIN_ALLOCATION_SIZE) return 0;
	else if ((Size % MIN_ALLOCATION_SIZE) == 0) return Size / MIN_ALLOCATION_SIZE - 1;
	return ((Size / MIN_ALLOCATION_SIZE + 1) * MIN_ALLOCATION_SIZE) / MIN_ALLOCATION_SIZE - 1;
}

size_t bit::SmallSizeAllocator::GetBlockSize(size_t BlockIndex)
{
	return MIN_ALLOCATION_SIZE * (BlockIndex + 1);
}

bit::SmallSizeAllocator::FreeBlock* bit::SmallSizeAllocator::PopFreeBlock(size_t BlockIndex)
{
	FreeBlock* Block = FreeLists[BlockIndex];
	if (Block != nullptr)
	{
		size_t BlockSize = GetBlockSize(BlockIndex);
		FreeLists[BlockIndex] = Block->Next;
#if BIT_DEBUG_MARK_BLOCKS
		bit::Memset(Block, 0xAA, BlockSize);
#endif
		UsedSpaceInBytes += BlockSize;
		Blocks[BlockIndex].Allocated += BlockSize;
		return Block;
	}
	return nullptr;
}

