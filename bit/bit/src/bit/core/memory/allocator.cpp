#include <bit/core/memory/allocator.h>
#include <bit/core/memory.h>
#include <bit/utility/utility.h>

bit::IAllocator::IAllocator(const char* InName)
{
#if BIT_ALLOCATOR_USE_NAME
	size_t NameLen = bit::Min(bit::Strlen(InName), ALLOCATOR_MAX_NAME_LEN - 1);
	bit::Memset(Name, 0, (ALLOCATOR_MAX_NAME_LEN));
	bit::Memcpy(Name, InName, NameLen);
#endif
}


bit::MemoryArena::~MemoryArena()
{
	if (RefCounter != nullptr && RefCounter->Decrement())
	{
		if (Allocator != nullptr)
		{
			Allocator->Free(BaseAddress);
			Allocator->Free(RefCounter);
		}
	}
}