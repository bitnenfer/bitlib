#pragma once

namespace bit
{
	template<typename TLockable>
	struct TScopedLock
	{
		TScopedLock(TLockable* InLockable) : Lockable(InLockable)
		{
			if (Lockable != nullptr) Lockable->Lock();
		}

		~TScopedLock()
		{
			if (Lockable != nullptr) Lockable->Unlock();
		}

	private:
		TLockable* Lockable;
	};
}
