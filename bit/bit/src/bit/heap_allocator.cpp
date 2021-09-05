#include <bit/heap_allocator.h>

bit::CHeapAllocator::CHeapAllocator(const char* Name) :
	bit::IAllocator(Name)
{}

bit::CHeapAllocator::~CHeapAllocator()
{

}

void* bit::CHeapAllocator::Allocate(size_t Size, size_t Alignment)
{
	return nullptr;
}

void* bit::CHeapAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	return nullptr;
}

void bit::CHeapAllocator::Free(void* Pointer)
{
}

size_t bit::CHeapAllocator::GetSize(void* Pointer)
{
	return size_t();
}

bit::CMemoryUsageInfo bit::CHeapAllocator::GetMemoryUsageInfo()
{
	return CMemoryUsageInfo();
}
