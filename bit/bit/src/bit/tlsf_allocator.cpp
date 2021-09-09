#include <bit/tlsf_allocator.h>

void* bit::CTLSFAllocator::Allocate(size_t Size, size_t Alignment)
{
	return nullptr;
}

void* bit::CTLSFAllocator::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	return nullptr;
}

void bit::CTLSFAllocator::Free(void* Pointer)
{
}

size_t bit::CTLSFAllocator::GetSize(void* Pointer)
{
	return size_t();
}

bit::CMemoryUsageInfo bit::CTLSFAllocator::GetMemoryUsageInfo()
{
	return CMemoryUsageInfo();
}
