#pragma once

#include <bit/types.h>
#include <bit/utils.h>

namespace bit
{
	struct BITLIB_API RWLock : NonCopyable, NonMovable
	{
		RWLock();
		~RWLock();
		void LockRead();
		void UnlockRead();
		void LockWrite();
		void UnlockWrite();
		bool TryLockRead();
		bool TryLockWrite();

	private:
		Handle_t Handle;
	};

	enum class RWLockType
	{
		LOCK_READ_ONLY,
		LOCK_READ_WRITE
	};

	struct BITLIB_API ScopedRWLock
	{
		ScopedRWLock(RWLock* InLockable, RWLockType InLockType);
		~ScopedRWLock();

	private:
		RWLock* Lockable;
		RWLockType LockType;
	};

	struct BITLIB_API ScopedReadLock
	{
		ScopedReadLock(RWLock* InLockable);
		~ScopedReadLock();

	private:
		RWLock* Lockable;
	};

	struct BITLIB_API ScopedWriteLock
	{
		ScopedWriteLock(RWLock* InLockable);
		~ScopedWriteLock();

	private:
		RWLock* Lockable;
	};
}
