#include <bit/core/os/debug.h>
#include <bit/core/os/thread_local_storage.h>
#include <bit/utility/scope_lock.h>
#include "../../windows_common.h"

bit::ThreadLocalStorage::ThreadLocalStorage() :
	Index(-1)
{}

bit::ThreadLocalStorage::ThreadLocalStorage(int32_t InIndex, void* InitialData) :
	Index(InIndex)
{
	if (IsValid())
	{
		SetData(InitialData);
	}
}

bool bit::ThreadLocalStorage::IsValid() const
{
	return Index != -1;
}

void bit::ThreadLocalStorage::SetData(void* InData)
{
	if (IsValid())
	{
		TlsSetValue(Index, InData);
	}
}

void* bit::ThreadLocalStorage::GetData()
{
	if (IsValid())
	{
		return (void*)TlsGetValue(Index);
	}
	return nullptr;
}

bit::ThreadLocalStorageHandle::ThreadLocalStorageHandle() : 
	InternalIndex(-1)
{}

bit::ThreadLocalStorageHandle::ThreadLocalStorageHandle(int32_t InIndex) :
	InternalIndex(InIndex)
{}

bit::ThreadLocalStorage* bit::ThreadLocalStorageHandle::operator->()
{
	if (InternalIndex != -1)
	{
		return &(ThreadLocalStorageAllocator::Get().GetAllTLSAllocs()[InternalIndex]);
	}
	return nullptr;
}

bit::ThreadLocalStorageHandle::operator bool()
{
	if (InternalIndex != -1)
	{
		return InternalIndex < ThreadLocalStorageAllocator::Get().GetAllTLSAllocs().GetCount();
	}
	return false;
}

bit::ThreadLocalStorageAllocator& bit::ThreadLocalStorageAllocator::Get()
{
	static bit::ThreadLocalStorageAllocator Instance{};
	return Instance;
}

bit::ThreadLocalStorageHandle bit::ThreadLocalStorageAllocator::Allocate(void* InitialData)
{
	bit::ScopedLock<Mutex> Lock(&StorageMutex);
	DWORD TLSIndex = TlsAlloc();
	BIT_ASSERT_MSG(TLSIndex != TLS_OUT_OF_INDEXES, "Ran out of thread local storage index space");
	if (TLSIndex != TLS_OUT_OF_INDEXES)
	{
		AllStorage.AddEmpty();
		BitPlacementNew(&AllStorage.GetLast()) ThreadLocalStorage(TLSIndex, InitialData);
		return ThreadLocalStorageHandle(AllStorage.GetCount() - 1);
	}
	return ThreadLocalStorageHandle(-1);
}

void bit::ThreadLocalStorageAllocator::Free(ThreadLocalStorageHandle TLSStorage)
{
	if (TLSStorage && TLSStorage->IsValid())
	{
		TlsFree(TLSStorage->Index);
		TLSStorage->Index = -1;
		// We can't delete any internal storage creation.
		// We'll keep this alive for the entirety of the program.
		//AllStorage.RemoveAt(TLSStorage.InternalIndex);
	}
}

bit::Array<bit::ThreadLocalStorage>& bit::ThreadLocalStorageAllocator::GetAllTLSAllocs()
{
	return AllStorage;
}

bool bit::operator==(const ThreadLocalStorage& LHS, const ThreadLocalStorage& RHS)
{
	return LHS.Index == RHS.Index;
}