#include <bit/linear_allocator.h>
#include <bit/utils.h>
#include <bit/memory.h>

namespace bit
{
	struct CLinearAllocatorHeader
	{
		size_t BlockSize;
		size_t RequestedSize;

		static CLinearAllocatorHeader* GetHeader(void* Ptr)
		{
			return (CLinearAllocatorHeader*)bit::OffsetPtr(Ptr, -(int64_t)sizeof(CLinearAllocatorHeader));
		}
	};
}

/*static*/ size_t bit::CLinearAllocator::GetRequiredAlignment()
{
	return alignof(CLinearAllocatorHeader);
}

bit::CLinearAllocator::CLinearAllocator(const char* Name, const CMemoryArena& Arena) :
	IAllocator::IAllocator(Name),
	Arena(Arena),
	BufferOffset(0)
{
}

bit::CLinearAllocator::~CLinearAllocator()
{
	Reset();
}

void bit::CLinearAllocator::Reset()
{
	BufferOffset = 0;
}

void* bit::CLinearAllocator::Allocate(size_t Size, size_t Alignment)
{
	uint8_t* BufferCurr = (uint8_t*)bit::OffsetPtr(Arena.GetBaseAddress(), BufferOffset);
	if ((size_t)bit::PtrDiff(BufferCurr, BufferCurr + Size + sizeof(CLinearAllocatorHeader)) < Arena.GetSizeInBytes())
	{
		void* NonAligned = BufferCurr + sizeof(CLinearAllocatorHeader);
		void* Aligned = bit::AlignPtr(NonAligned, Alignment);
		size_t TotalSize = bit::PtrDiff(BufferCurr, bit::OffsetPtr(Aligned, Size));
		CLinearAllocatorHeader* Header = CLinearAllocatorHeader::GetHeader(Aligned);
		Header->BlockSize = TotalSize;
		Header->RequestedSize = Size;
		BufferOffset += TotalSize;
		return Aligned;
	}
	return nullptr;
}

void* bit::CLinearAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
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
	bit::Memcpy(NewPtr, Pointer, bit::CLinearAllocatorHeader::GetHeader(Pointer)->RequestedSize);
	Free(Pointer);
	return NewPtr;
}

void bit::CLinearAllocator::Free(void* Pointer) 
{ 
	CLinearAllocatorHeader* Header = CLinearAllocatorHeader::GetHeader(Pointer);
	void* PossiblePrevAddress = bit::OffsetPtr(bit::OffsetPtr(Arena.GetBaseAddress(), BufferOffset), -(int64_t)Header->BlockSize);
	if (Header == PossiblePrevAddress)
	{
		BufferOffset -= Header->BlockSize;
	}
}

size_t bit::CLinearAllocator::GetSize(void* Pointer)
{
	return CLinearAllocatorHeader::GetHeader(Pointer)->RequestedSize;
}

bit::CMemoryUsageInfo bit::CLinearAllocator::GetMemoryUsageInfo()
{
	CMemoryUsageInfo Info = {};
	Info.AllocatedBytes = (size_t)bit::PtrDiff(bit::OffsetPtr(Arena.GetBaseAddress(), BufferOffset), Arena.GetBaseAddress());
	Info.CommittedBytes = Arena.GetSizeInBytes();
	Info.ReservedBytes = Arena.GetSizeInBytes();
	return Info;
}
