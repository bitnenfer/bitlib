#include <bit/core/memory.h>

double bit::FromKiB(size_t Value) { return (double)Value / 1024.0; }
double bit::FromMiB(size_t Value) { return FromKiB(Value) / 1024.0; }
double bit::FromGiB(size_t Value) { return FromMiB(Value) / 1024.0; }
double bit::FromTiB(size_t Value) { return FromGiB(Value) / 1024.0; }

void* bit::Malloc(size_t Size, size_t Alignment)
{
	return GetDefaultAllocator().Allocate(Size, Alignment);
}

void* bit::Realloc(void* Pointer, size_t Size, size_t Alignment)
{
	return GetDefaultAllocator().Reallocate(Pointer, Size, Alignment);
}

void bit::Free(void* Pointer)
{
	return GetDefaultAllocator().Free(Pointer);
}