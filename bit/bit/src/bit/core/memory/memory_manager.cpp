#include <bit/core/memory/memory_manager.h>
#include <bit/core/os/debug.h>
#include <bit/core/memory.h>
#include <bit/utility/scope_lock.h>

bit::MemoryManager::MemoryManager() :
	IAllocator("MemoryManager"),
	SmallAllocator(),
	MediumAllocator()
{
}

void* bit::MemoryManager::Allocate(size_t Size, size_t Alignment)
{
	bit::ScopedLock<bit::Mutex> Lock(&AccessLock);
	if (SmallAllocator.CanAllocate(Size, Alignment))
	{
		Small++;
		return SmallAllocator.Allocate(Size, Alignment);
	}
	else if (MediumAllocator.CanAllocate(Size, Alignment))
	{
		Medium++;
		return MediumAllocator.Allocate(Size, Alignment);
	}
	return nullptr;
}
void* bit::MemoryManager::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr) return Allocate(Size, Alignment);
	size_t BlockSize = GetSize(Pointer);
	if (BlockSize >= Size && BlockSize - Size < SmallSizeAllocator::MIN_ALLOCATION_SIZE) 
		return Pointer;
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
}
size_t bit::MemoryManager::GetSize(void* Pointer)
{
	if (SmallAllocator.OwnsAllocation(Pointer)) return SmallAllocator.GetSize(Pointer);
	else if (MediumAllocator.OwnsAllocation(Pointer)) return MediumAllocator.GetSize(Pointer);
	return 0;
}
bit::AllocatorMemoryInfo bit::MemoryManager::GetMemoryUsageInfo()
{
	bit::ScopedLock<bit::Mutex> Lock(&AccessLock);
	AllocatorMemoryInfo SmallUsage = SmallAllocator.GetMemoryUsageInfo();
	AllocatorMemoryInfo MediumUsage = MediumAllocator.GetMemoryUsageInfo();
	AllocatorMemoryInfo Usage = {};
	Usage.AllocatedBytes = SmallUsage.AllocatedBytes + MediumUsage.AllocatedBytes;
	Usage.CommittedBytes = SmallUsage.CommittedBytes + MediumUsage.CommittedBytes;
	Usage.ReservedBytes = SmallUsage.ReservedBytes + MediumUsage.ReservedBytes;
	//BIT_ALWAYS_LOG(
	//	"Small Allocs: %u\n"
	//	"Medium Allocs: %u\n"
	//	"Heap Allocs: %u\n",
	//	Small, Medium, Heap
	//);
	return Usage;
}

bool bit::MemoryManager::CanAllocate(size_t Size, size_t Alignment)
{
	return SmallAllocator.CanAllocate(Size, Alignment) 
		|| MediumAllocator.CanAllocate(Size, Alignment);
}

bool bit::MemoryManager::OwnsAllocation(const void* Ptr)
{
	return SmallAllocator.OwnsAllocation(Ptr) 
		|| MediumAllocator.OwnsAllocation(Ptr);
}

size_t bit::MemoryManager::Compact()
{
	return SmallAllocator.Compact() + MediumAllocator.Compact();
}

namespace bit
{
	static uint8_t HeapInitialBuffer[sizeof(MemoryManager)];
	bit::IAllocator* CreateGlobalAllocator() { return BitPlacementNew(HeapInitialBuffer) MemoryManager(); }
	static IAllocator* GlobalAllocator = nullptr;
}

bit::IAllocator& bit::GetGlobalAllocator()
{
	if (GlobalAllocator == nullptr)
	{
		GlobalAllocator = CreateGlobalAllocator();
	}
	return *GlobalAllocator;
}
