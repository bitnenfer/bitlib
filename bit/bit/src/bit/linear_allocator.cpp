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
	return sizeof(CLinearAllocatorHeader);
}

bit::CLinearAllocator::CLinearAllocator(const char* Name) :
	IAllocator::IAllocator(Name),
	Buffer(nullptr),
	BufferOffset(0),
	BufferSize(0)
{
}

bit::CLinearAllocator::~CLinearAllocator()
{
	Terminate();
}

void bit::CLinearAllocator::Initialize(void* InBuffer, size_t InBufferSize)
{
	Terminate();
	Buffer = (uint8_t*)InBuffer;
	BufferSize = InBufferSize;
}

void bit::CLinearAllocator::Terminate()
{
	Buffer = nullptr;
	BufferOffset = 0;
	BufferSize = 0;
}

void bit::CLinearAllocator::Reset()
{
	BufferOffset = 0;
}

void* bit::CLinearAllocator::GetBuffer(size_t* OutSize)
{
	if (OutSize != nullptr) *OutSize = BufferSize;
	return Buffer;
}

void* bit::CLinearAllocator::Alloc(size_t Size, size_t Alignment)
{
	if ((size_t)bit::PtrDiff(Buffer, Buffer + BufferOffset + Size) < BufferSize)
	{
		void* NonAligned = bit::ForwardPtr(Buffer, BufferOffset + sizeof(CLinearAllocatorHeader));
		void* Aligned = bit::AlignPtr(NonAligned, Alignment);
		CLinearAllocatorHeader::GetHeader(Aligned)->BlockSize = Size;
		BufferOffset += bit::PtrDiff(Buffer, Aligned);
		return Aligned;
	}
	return nullptr;
}

void* bit::CLinearAllocator::Realloc(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr)
	{
		return Alloc(Size, Alignment);
	}
	else if (Size == 0)
	{
		Free(Pointer);
		return nullptr;
	}
	void* NewPtr = Alloc(Size, Alignment);
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
	Info.AllocatedBytes = (size_t)bit::PtrDiff(bit::ForwardPtr(Buffer, BufferOffset), Buffer);
	Info.CommittedBytes = BufferSize;
	Info.ReservedBytes = BufferSize;
	return Info;
}
