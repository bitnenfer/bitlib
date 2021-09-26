#include <bit/core/memory/linear_allocator.h>
#include <bit/utility/utility.h>
#include <bit/core/memory.h>

namespace bit
{
	struct LinearAllocatorHeader
	{
		size_t BlockSize;
		size_t RequestedSize;

		static LinearAllocatorHeader* GetHeader(void* Ptr)
		{
			return (LinearAllocatorHeader*)bit::OffsetPtr(Ptr, -(intptr_t)sizeof(LinearAllocatorHeader));
		}
	};
}

/*static*/ size_t bit::LinearAllocator::GetRequiredAlignment()
{
	return alignof(LinearAllocatorHeader);
}

bit::LinearAllocator::LinearAllocator(const char* Name, const MemoryArena& Arena) :
	IAllocator::IAllocator(Name),
	Arena(Arena),
	BufferOffset(0)
{
}

bit::LinearAllocator::~LinearAllocator()
{
	Reset();
}

void bit::LinearAllocator::Reset()
{
	BufferOffset = 0;
}

void* bit::LinearAllocator::Allocate(size_t Size, size_t Alignment)
{
	uint8_t* BufferCurr = (uint8_t*)bit::OffsetPtr(Arena.GetBaseAddress(), BufferOffset);
	if ((size_t)bit::PtrDiff(BufferCurr, BufferCurr + Size + sizeof(LinearAllocatorHeader)) < Arena.GetSizeInBytes())
	{
		void* NonAligned = BufferCurr + sizeof(LinearAllocatorHeader);
		void* Aligned = bit::AlignPtr(NonAligned, Alignment);
		size_t TotalSize = bit::PtrDiff(BufferCurr, bit::OffsetPtr(Aligned, Size));
		LinearAllocatorHeader* Header = LinearAllocatorHeader::GetHeader(Aligned);
		Header->BlockSize = TotalSize;
		Header->RequestedSize = Size;
		BufferOffset += TotalSize;
		return Aligned;
	}
	return nullptr;
}

void* bit::LinearAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr)
	{
		return Allocate(Size, Alignment);
	}
	else if (Size == 0)
	{
		Free(Pointer);
		return nullptr;
	}
	void* NewPtr = Allocate(Size, Alignment);
	bit::Memcpy(NewPtr, Pointer, bit::LinearAllocatorHeader::GetHeader(Pointer)->RequestedSize);
	Free(Pointer);
	return NewPtr;
}

void bit::LinearAllocator::Free(void* Pointer) 
{ 
	LinearAllocatorHeader* Header = LinearAllocatorHeader::GetHeader(Pointer);
	void* PossiblePrevAddress = bit::OffsetPtr(bit::OffsetPtr(Arena.GetBaseAddress(), BufferOffset), -(intptr_t)Header->BlockSize);
	if (Header == PossiblePrevAddress)
	{
		BufferOffset -= Header->BlockSize;
	}
}

size_t bit::LinearAllocator::GetSize(void* Pointer)
{
	return LinearAllocatorHeader::GetHeader(Pointer)->RequestedSize;
}

bit::MemoryUsageInfo bit::LinearAllocator::GetMemoryUsageInfo()
{
	MemoryUsageInfo Info = {};
	Info.AllocatedBytes = (size_t)bit::PtrDiff(bit::OffsetPtr(Arena.GetBaseAddress(), BufferOffset), Arena.GetBaseAddress());
	Info.CommittedBytes = Arena.GetSizeInBytes();
	Info.ReservedBytes = Arena.GetSizeInBytes();
	return Info;
}
