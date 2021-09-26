#include <bit/allocator.h>
#include <bit/memory.h>
#include <bit/utils.h>

bit::Allocator::Allocator(const char* InName)
{
	size_t NameLen = bit::Min(bit::Strlen(InName), ALLOCATOR_MAX_NAME_LEN - 1);
	bit::Memset(Name, 0, (ALLOCATOR_MAX_NAME_LEN));
	bit::Memcpy(Name, InName, NameLen);
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