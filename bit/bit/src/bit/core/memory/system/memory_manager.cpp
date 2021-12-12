#include <bit/core/memory/system/memory_manager.h>
#include <bit/core/os/debug.h>
#include <bit/core/memory.h>
#include <bit/utility/scope_lock.h>

bit::MemoryManager::MemoryManager() :
	IAllocator("MemoryManager"),
	BaseAllocator()
{
}

void* bit::MemoryManager::Allocate(size_t Size, size_t Alignment)
{
	bit::ScopedLock<bit::Mutex> Lock(&AccessLock);
	if (BaseAllocator.CanAllocate(Size, Alignment))
	{
		return BaseAllocator.Allocate(Size, Alignment);
	}
	return nullptr;
}
void* bit::MemoryManager::Reallocate(void* Pointer, size_t Size, size_t Alignment)
{
	if (Pointer == nullptr) return Allocate(Size, Alignment);
	size_t BlockSize = GetSize(Pointer);
	void* NewBlock = Allocate(Size, Alignment);
	bit::Memcpy(NewBlock, Pointer, bit::Min(BlockSize, Size));
	Free(Pointer);
	return NewBlock;
}
void bit::MemoryManager::Free(void* Pointer)
{
	bit::ScopedLock<bit::Mutex> Lock(&AccessLock);
	if (BaseAllocator.OwnsAllocation(Pointer))
	{
		BaseAllocator.Free(Pointer);
	}
}
size_t bit::MemoryManager::GetSize(void* Pointer)
{
	if (BaseAllocator.OwnsAllocation(Pointer)) return BaseAllocator.GetSize(Pointer);
	return 0;
}
bit::AllocatorMemoryInfo bit::MemoryManager::GetMemoryUsageInfo()
{
	bit::ScopedLock<bit::Mutex> Lock(&AccessLock);
	AllocatorMemoryInfo BaseUsage = BaseAllocator.GetMemoryUsageInfo();
	AllocatorMemoryInfo Usage = {};
	Usage.AllocatedBytes = BaseUsage.AllocatedBytes;
	Usage.CommittedBytes = BaseUsage.CommittedBytes;
	Usage.ReservedBytes = BaseUsage.ReservedBytes;
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
	return BaseAllocator.CanAllocate(Size, Alignment);
}

bool bit::MemoryManager::OwnsAllocation(const void* Ptr)
{
	return BaseAllocator.OwnsAllocation(Ptr);
}

size_t bit::MemoryManager::Compact()
{
	return BaseAllocator.Compact();
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
