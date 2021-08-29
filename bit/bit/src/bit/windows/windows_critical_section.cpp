#include <bit/critical_section.h>
#include <bit/memory.h>
#include "windows_common.h"

bit::CCriticalSection::CCriticalSection() :
	Handle(nullptr)
{
	Handle = Malloc(sizeof(CRITICAL_SECTION), bit::DEFAULT_ALIGNMENT);
	InitializeCriticalSection((CRITICAL_SECTION*)Handle);
}

bit::CCriticalSection::~CCriticalSection()
{
	DeleteCriticalSection((CRITICAL_SECTION*)Handle);
	Free(Handle);
}

void bit::CCriticalSection::Lock()
{
	EnterCriticalSection((CRITICAL_SECTION*)Handle);
}

void bit::CCriticalSection::Unlock()
{
	LeaveCriticalSection((CRITICAL_SECTION*)Handle);
}

bool bit::CCriticalSection::TryLock()
{
	return TryEnterCriticalSection((CRITICAL_SECTION*)Handle) == TRUE;
}

bit::CScopeLock::CScopeLock(bit::CCriticalSection* CS) :
	CS(CS)
{
	if (CS != nullptr) CS->Lock();
}

bit::CScopeLock::~CScopeLock()
{
	if (CS != nullptr) CS->Unlock();
}