#include <bit/core/memory/small_allocator.h>
#include <bit/core/memory.h>
#include <bit/core/os/debug.h>

bit::SmallAllocator::SmallAllocator(size_t ReservedSpaceInBytes, const char* Name) :
	IAllocator(Name),
	UsedSpaceInBytes(0)
{
	BIT_ASSERT((ReservedSpaceInBytes % NUM_OF_SIZES) == 0);

	if (!VirtualReserveSpace(VirtualRandomAddress(), ReservedSpaceInBytes, Memory))
	{
		BIT_PANIC();
	}

	size_t RangeSize = ReservedSpaceInBytes / NUM_OF_SIZES;
	void* BaseAddress = bit::AlignPtr(Memory.GetBaseAddress(), 8);
	void* NextAddressSize = bit::OffsetPtr(BaseAddress, RangeSize);

	for (uint32_t Index = 0; Index < NUM_OF_SIZES; ++Index)
	{
		AddressRanges[Index].Start = BaseAddress;
		AddressRanges[Index].End = NextAddressSize;
		BaseAddress = NextAddressSize;
		NextAddressSize = bit::OffsetPtr(BaseAddress, RangeSize);
	}
}

bit::SmallAllocator::SmallAllocator(const char* Name) :
	IAllocator(Name),
	UsedSpaceInBytes(0)
{
}

void* bit::SmallAllocator::Allocate(size_t Size, size_t Alignment)
{
	size_t AlignedSize = bit::AlignUint(Size, bit::Max(MIN_ALLOCATION_SIZE, Alignment));
	uint32_t Index = (uint32_t)AlignedSize / MIN_ALLOCATION_SIZE - 1;
	FreeBlock* Block = FreeLists[Index];
	if (Block != nullptr)
	{
		FreeLists[Index] = Block->Next;
		return Block;
	}

	return nullptr;
}

void* bit::SmallAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr) return Allocate(Size, Alignment);
	size_t BlockSize = GetSize(Pointer);
	if (BlockSize == Size) return Pointer;
	void* NewBlock = Allocate(Size, Alignment);
	bit::Memcpy(NewBlock, Pointer, bit::Min(BlockSize, Size));
	Free(Pointer);
	return NewBlock;
}

void bit::SmallAllocator::Free(void* Pointer)
{
	size_t Size = GetSize(Pointer);
	if (Size > 0)
	{
		FreeBlock* Block = reinterpret_cast<FreeBlock*>(Pointer);
		uint32_t Index = (uint32_t)Size / MIN_ALLOCATION_SIZE - 1;
		Block->Next = FreeLists[Index];
		FreeLists[Index] = Block;
	}
}

size_t bit::SmallAllocator::GetSize(void* Pointer)
{
	for (uint32_t Index = 0; Index < NUM_OF_SIZES; ++Index)
	{
		if (AddressRanges[Index].OwnsPointer(Pointer))
		{
			return MIN_ALLOCATION_SIZE * (Index + 1);
		}
	}
	return 0;
}

bit::MemoryUsageInfo bit::SmallAllocator::GetMemoryUsageInfo()
{
	MemoryUsageInfo Usage = {};
	Usage.ReservedBytes = Memory.GetRegionSize();
	Usage.CommittedBytes = Memory.GetCommittedSize();
	Usage.AllocatedBytes = UsedSpaceInBytes;
	return Usage;
}
