#include <bit/rw_lock.h>
#include "windows_common.h"

bit::CRWLock::CRWLock()
{
	Handle = nullptr;
	InitializeSRWLock((PSRWLOCK)&Handle);
}

bit::CRWLock::~CRWLock()
{}

void bit::CRWLock::LockShared()
{
	AcquireSRWLockShared((PSRWLOCK)&Handle);
}

void bit::CRWLock::UnlockShared()
{
	ReleaseSRWLockShared((PSRWLOCK)&Handle);
}

void bit::CRWLock::LockExclusive()
{
	AcquireSRWLockExclusive((PSRWLOCK)&Handle);
}

void bit::CRWLock::UnlockExclusive()
{
	ReleaseSRWLockExclusive((PSRWLOCK)&Handle);
}

bool bit::CRWLock::TryLockShared()
{
	return TryAcquireSRWLockShared((PSRWLOCK)&Handle) == TRUE;
}

bool bit::CRWLock::TryLockExclusive()
{
	return TryAcquireSRWLockExclusive((PSRWLOCK)&Handle) == TRUE;
}
