#include <bit/core/memory.h>
#include <bit/core/os/debug.h>

double bit::FromKiB(size_t Value) { return (double)Value / 1024.0; }
double bit::FromMiB(size_t Value) { return FromKiB(Value) / 1024.0; }
double bit::FromGiB(size_t Value) { return FromMiB(Value) / 1024.0; }
double bit::FromTiB(size_t Value) { return FromGiB(Value) / 1024.0; }

void* bit::Malloc(size_t Size, size_t Alignment)
{
	return GetGlobalAllocator().Allocate(Size, Alignment);
}

void* bit::Realloc(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr)
	{
		return Malloc(Size, Alignment);
	}
	if (Size == 0)
	{
		Free(Pointer);
		return nullptr;
	}
	size_t OldSize = GetMallocSize(Pointer);
	if (OldSize == 0)
	{
		BIT_PANIC_MSG("Invalid pointer to realloc");
		return nullptr;
	}
	if (Size <= OldSize && (OldSize - Size) < sizeof(void*))
	{
		return Pointer;
	}
	void* NewBlock = Malloc(Size, Alignment);
	Memcpy(NewBlock, Pointer, Min(Size, OldSize));
	Free(Pointer);
	return NewBlock;
}

void bit::Free(void* Pointer)
{
	return GetGlobalAllocator().Free(Pointer);
}

size_t bit::CompactMemory()
{
	return GetGlobalAllocator().Compact();
}

size_t bit::GetMallocSize(void* Pointer)
{
	return GetGlobalAllocator().GetSize(Pointer);
}