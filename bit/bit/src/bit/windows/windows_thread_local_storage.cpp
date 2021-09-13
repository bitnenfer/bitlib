#include <bit/os.h>
#include <bit/thread_local_storage.h>
#include <bit/scope_lock.h>
#include "windows_common.h"

bit::CTLStorage::CTLStorage() :
	Index(-1)
{}

bit::CTLStorage::CTLStorage(int32_t InIndex, void* InitialData) :
	Index(InIndex)
{
	if (IsValid())
	{
		SetData(InitialData);
	}
}

bool bit::CTLStorage::IsValid() const
{
	return Index != -1;
}

void bit::CTLStorage::SetData(void* InData)
{
	if (IsValid())
	{
		TlsSetValue(Index, InData);
	}
}

void* bit::CTLStorage::GetData()
{
	if (IsValid())
	{
		return (void*)TlsGetValue(Index);
	}
	return nullptr;
}

bit::CTLSHandle::CTLSHandle() : 
	InternalIndex(-1)
{}

bit::CTLSHandle::CTLSHandle(int32_t InIndex) :
	InternalIndex(InIndex)
{}

bit::CTLStorage* bit::CTLSHandle::operator->()
{
	if (InternalIndex != -1)
	{
		return &(CTLSAllocator::Get().GetAllTLSAllocs()[InternalIndex]);
	}
	return nullptr;
}

bit::CTLSHandle::operator bool()
{
	if (InternalIndex != -1)
	{
		return InternalIndex < CTLSAllocator::Get().GetAllTLSAllocs().GetCount();
	}
	return false;
}

bit::CTLSAllocator& bit::CTLSAllocator::Get()
{
	static bit::CTLSAllocator Instance{};
	return Instance;
}

bit::CTLSHandle bit::CTLSAllocator::Allocate(void* InitialData)
{
	bit::TScopedLock<CMutex> Lock(&StorageMutex);
	DWORD TLSIndex = TlsAlloc();
	BIT_ASSERT_MSG(TLSIndex != TLS_OUT_OF_INDEXES, "Ran out of thread local storage index space");
	if (TLSIndex != TLS_OUT_OF_INDEXES)
	{
		AllStorage.AddEmpty();
		BitPlacementNew(&AllStorage.GetLast()) CTLStorage(TLSIndex, InitialData);
		return CTLSHandle(AllStorage.GetCount() - 1);
	}
	return CTLSHandle(-1);
}

void bit::CTLSAllocator::Free(CTLSHandle TLSStorage)
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

bit::TArray<bit::CTLStorage>& bit::CTLSAllocator::GetAllTLSAllocs()
{
	return AllStorage;
}

bool bit::operator==(const CTLStorage& LHS, const CTLStorage& RHS)
{
	return LHS.Index == RHS.Index;
}