#include <bit/core/os/rw_lock.h>

bit::ScopedRWLock::ScopedRWLock(RWLock* InLockable, RWLockType InLockType) :
	Lockable(InLockable),
	LockType(InLockType)
{
	if (Lockable != nullptr)
	{
		if (LockType == RWLockType::LOCK_READ_ONLY)
		{
			Lockable->LockRead();
		}
		else
		{
			Lockable->LockWrite();
		}
	}
}

bit::ScopedRWLock::~ScopedRWLock()
{
	if (Lockable != nullptr)
	{
		if (LockType == RWLockType::LOCK_READ_ONLY)
		{
			Lockable->UnlockRead();
		}
		else
		{
			Lockable->UnlockWrite();
		}
	}
}

bit::ScopedReadLock::ScopedReadLock(RWLock* InLockable) :
	Lockable(InLockable)
{
	if (Lockable != nullptr)
	{
		Lockable->LockRead();
	}
}

bit::ScopedReadLock::~ScopedReadLock()
{
	if (Lockable != nullptr)
	{
		Lockable->UnlockRead();
	}
}

bit::ScopedWriteLock::ScopedWriteLock(RWLock* InLockable) :
	Lockable(InLockable)
{
	if (Lockable != nullptr)
	{
		Lockable->LockWrite();
	}
}

bit::ScopedWriteLock::~ScopedWriteLock()
{
	if (Lockable != nullptr)
	{
		Lockable->UnlockWrite();
	}
}
