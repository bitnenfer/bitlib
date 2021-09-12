#pragma once

#include <bit/types.h>
#include <bit/utils.h>

namespace bit
{
	struct BITLIB_API CRWLock : CNonCopyable, CNonMovable
	{
		CRWLock();
		~CRWLock();
		void LockShared();
		void UnlockShared();
		void LockExclusive();
		void UnlockExclusive();
		bool TryLockShared();
		bool TryLockExclusive();

	private:
		Handle_t Handle;
	};

	enum class ERWLockType
	{
		LOCK_READ_ONLY,
		LOCK_READ_WRITE
	};

	struct BITLIB_API CScopedRWLock
	{
		CScopedRWLock(CRWLock* InLockable, ERWLockType InLockType);
		~CScopedRWLock();

	private:
		CRWLock* Lockable;
		ERWLockType LockType;
	};
}
