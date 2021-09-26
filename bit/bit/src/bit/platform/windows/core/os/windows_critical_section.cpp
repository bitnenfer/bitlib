#include <bit/core/os/critical_section.h>
#include <bit/core/memory.h>
#include "../../windows_common.h"

bit::CriticalSection::CriticalSection() :
	Handle(nullptr)
{
	Handle = Malloc(sizeof(CRITICAL_SECTION), bit::DEFAULT_ALIGNMENT);
	InitializeCriticalSection((CRITICAL_SECTION*)Handle);
}

bit::CriticalSection::~CriticalSection()
{
	DeleteCriticalSection((CRITICAL_SECTION*)Handle);
	Free(Handle);
}

void bit::CriticalSection::Lock()
{
	EnterCriticalSection((CRITICAL_SECTION*)Handle);
}

void bit::CriticalSection::Unlock()
{
	LeaveCriticalSection((CRITICAL_SECTION*)Handle);
}

bool bit::CriticalSection::TryLock()
{
	return TryEnterCriticalSection((CRITICAL_SECTION*)Handle) == TRUE;
}
