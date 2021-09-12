#include <bit/rw_lock.h>

bit::CScopedRWLock::CScopedRWLock(CRWLock* InLockable, ERWLockType InLockType) :
	Lockable(InLockable),
	LockType(InLockType)
{
	if (Lockable != nullptr)
	{
		if (LockType == ERWLockType::LOCK_READ_ONLY)
		{
			Lockable->LockShared();
		}
		else
		{
			Lockable->LockExclusive();
		}
	}
}

bit::CScopedRWLock::~CScopedRWLock()
{
	if (Lockable != nullptr)
	{
		if (LockType == ERWLockType::LOCK_READ_ONLY)
		{
			Lockable->UnlockShared();
		}
		else
		{
			Lockable->UnlockExclusive();
		}
	}
}
