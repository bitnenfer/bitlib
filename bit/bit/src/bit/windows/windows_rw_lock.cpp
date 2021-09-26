#include <bit/rw_lock.h>
#include "windows_common.h"

bit::RWLock::RWLock()
{
	Handle = nullptr;
	InitializeSRWLock((PSRWLOCK)&Handle);
}

bit::RWLock::~RWLock()
{}

void bit::RWLock::LockRead()
{
	AcquireSRWLockShared((PSRWLOCK)&Handle);
}

void bit::RWLock::UnlockRead()
{
	ReleaseSRWLockShared((PSRWLOCK)&Handle);
}

void bit::RWLock::LockWrite()
{
	AcquireSRWLockExclusive((PSRWLOCK)&Handle);
}

void bit::RWLock::UnlockWrite()
{
	ReleaseSRWLockExclusive((PSRWLOCK)&Handle);
}

bool bit::RWLock::TryLockRead()
{
	return TryAcquireSRWLockShared((PSRWLOCK)&Handle) == TRUE;
}

bool bit::RWLock::TryLockWrite()
{
	return TryAcquireSRWLockExclusive((PSRWLOCK)&Handle) == TRUE;
}
