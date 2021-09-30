#include <bit/core/memory/memory_manager.h>
#include <bit/core/os/debug.h>
#include <bit/core/memory.h>
#include <bit/utility/scope_lock.h>

bit::MemoryManager::MemoryManager() :
	IAllocator("MemoryManager"),
	SmallAllocator("MemoryManager::SmallAllocator"),
	MediumAllocator("MemoryManager::MediumAllocator")
{
}

void* bit::MemoryManager::Allocate(size_t Size, size_t Alignment)
{
	bit::ScopedLock<bit::Mutex> Lock(&AccessLock);
	if (SmallAllocator.CanAllocate(Size, Alignment))
	{
		return SmallAllocator.Allocate(Size, Alignment);
	}
	return MediumAllocator.Allocate(Size, Alignment);
}
void* bit::MemoryManager::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr) return Allocate(Size, Alignment);
	size_t BlockSize = GetSize(Pointer);
	if (BlockSize == Size) return Pointer;
	void* NewBlock = Allocate(Size, Alignment);
	bit::Memcpy(NewBlock, Pointer, bit::Min(BlockSize, Size));
	Free(Pointer);
	return NewBlock;
}
void bit::MemoryManager::Free(void* Pointer)
{
	bit::ScopedLock<bit::Mutex> Lock(&AccessLock);
	if (SmallAllocator.OwnsAllocation(Pointer))
	{
		SmallAllocator.Free(Pointer);
	}
	else if (MediumAllocator.OwnsAllocation(Pointer))
	{
		MediumAllocator.Free(Pointer);
	}
	else
	{
		BIT_PANIC(); // Invalid Pointer ?
	}
}
size_t bit::MemoryManager::GetSize(void* Pointer)
{
	size_t SmallSize = SmallAllocator.GetSize(Pointer);
	if (SmallSize == 0) return MediumAllocator.GetSize(Pointer);
	return SmallSize;
}
bit::MemoryUsageInfo bit::MemoryManager::GetMemoryUsageInfo()
{
	bit::ScopedLock<bit::Mutex> Lock(&AccessLock);
	MemoryUsageInfo SmallUsage =  SmallAllocator.GetMemoryUsageInfo();
	MemoryUsageInfo MediumUsage = MediumAllocator.GetMemoryUsageInfo();
	MemoryUsageInfo Usage = {};
	Usage.AllocatedBytes = SmallUsage.AllocatedBytes + MediumUsage.AllocatedBytes;
	Usage.CommittedBytes = SmallUsage.CommittedBytes + MediumUsage.CommittedBytes;
	Usage.ReservedBytes = SmallUsage.ReservedBytes + MediumUsage.ReservedBytes;
	return Usage;
}

bool bit::MemoryManager::CanAllocate(size_t Size, size_t Alignment)
{
	return SmallAllocator.CanAllocate(Size, Alignment) || SmallAllocator.CanAllocate(Size, Alignment);
}

bool bit::MemoryManager::OwnsAllocation(const void* Ptr)
{
	return SmallAllocator.OwnsAllocation(Ptr) || MediumAllocator.OwnsAllocation(Ptr);
}

size_t bit::MemoryManager::Compact()
{
	return SmallAllocator.Compact() + MediumAllocator.Compact();
}

#if !BIT_PLATFORM_DEFAULT_ALLOCATOR
namespace bit
{
	static uint8_t HeapInitialBuffer[sizeof(MemoryManager)];
	bit::IAllocator* CreateDefaultAllocator() { return BitPlacementNew(HeapInitialBuffer) MemoryManager(); }
	static IAllocator* DefaultAllocator = nullptr;
}

bit::IAllocator& bit::GetDefaultAllocator()
{
	if (DefaultAllocator == nullptr)
	{
		DefaultAllocator = CreateDefaultAllocator();
	}
	return *DefaultAllocator;
}
#endif
