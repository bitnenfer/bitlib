#pragma once

#include <bit/types.h>
#include <bit/utils.h>

namespace bit
{
	struct BITLIB_API CriticalSection : public NonCopyable
	{
		CriticalSection();
		~CriticalSection();

		void Lock();
		void Unlock();
		bool TryLock();

	private:
		Handle_t Handle;
	};
}
