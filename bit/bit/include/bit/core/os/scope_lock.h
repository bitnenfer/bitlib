#pragma once

namespace bit
{
	template<typename TLockable>
	struct ScopedLock
	{
		ScopedLock(TLockable* InLockable) : Lockable(InLockable)
		{
			if (Lockable != nullptr) Lockable->Lock();
		}

		~ScopedLock()
		{
			if (Lockable != nullptr) Lockable->Unlock();
		}

	private:
		TLockable* Lockable;
	};
}
