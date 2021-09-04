#include <bit/linear_allocator.h>
#include <bit/utils.h>
#include <bit/memory.h>

namespace bit
{
	struct CLinearAllocatorHeader
	{
		size_t BlockSize;

		static CLinearAllocatorHeader* GetHeader(void* Ptr)
		{
			return (CLinearAllocatorHeader*)bit::BackwardPtr(Ptr, sizeof(CLinearAllocatorHeader));
		}
	};
}

/*static*/ size_t bit::CLinearAllocator::GetRequiredAlignment()
{
	return alignof(CLinearAllocatorHeader);
}

bit::CLinearAllocator::CLinearAllocator(const char* Name) :
	IAllocator::IAllocator(Name),
	Arena({nullptr, 0}),
	BufferOffset(0)
{
}

bit::CLinearAllocator::~CLinearAllocator()
{
	Terminate();
}

void bit::CLinearAllocator::Initialize(bit::CMemoryArena InArena)
{
	Terminate();
	Arena = InArena;
}

void bit::CLinearAllocator::Terminate()
{
	Arena.BaseAddress = nullptr;
	Arena.SizeInBytes = 0;
	BufferOffset = 0;
}

void bit::CLinearAllocator::Reset()
{
	BufferOffset = 0;
}

void* bit::CLinearAllocator::GetBuffer(size_t* OutSize)
{
	if (OutSize != nullptr) *OutSize = Arena.SizeInBytes;
	return Arena.BaseAddress;
}

void* bit::CLinearAllocator::Allocate(size_t Size, size_t Alignment)
{
	uint8_t* BufferCurr = (uint8_t*)bit::ForwardPtr(Arena.BaseAddress, BufferOffset);
	if ((size_t)bit::PtrDiff(BufferCurr, BufferCurr + Size + sizeof(CLinearAllocatorHeader)) < Arena.SizeInBytes)
	{
		void* NonAligned = BufferCurr + sizeof(CLinearAllocatorHeader);
		void* Aligned = bit::AlignPtr(NonAligned, Alignment);
		CLinearAllocatorHeader::GetHeader(Aligned)->BlockSize = Size;
		BufferOffset += bit::PtrDiff(BufferCurr, bit::ForwardPtr(Aligned, Size));
		bit::Memset(Aligned, 0xAA, Size);
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
	bit::Memcpy(NewPtr, Pointer, bit::CLinearAllocatorHeader::GetHeader(Pointer)->BlockSize);
	Free(Pointer);
	return NewPtr;
}

void bit::CLinearAllocator::Free(void* Pointer) { /* Do nothing */ }

size_t bit::CLinearAllocator::GetSize(void* Pointer)
{
	return CLinearAllocatorHeader::GetHeader(Pointer)->BlockSize;
}

bit::CMemoryInfo bit::CLinearAllocator::GetMemoryInfo()
{
	CMemoryInfo Info = {};
	Info.AllocatedBytes = (size_t)bit::PtrDiff(bit::ForwardPtr(Arena.BaseAddress, BufferOffset), Arena.BaseAddress);
	Info.CommittedBytes = Arena.SizeInBytes;
	Info.ReservedBytes = Arena.SizeInBytes;
	return Info;
}
