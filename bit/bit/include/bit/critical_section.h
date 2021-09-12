#pragma once

#include <bit/types.h>
#include <bit/utils.h>

namespace bit
{
	struct BITLIB_API CCriticalSection : public CNonCopyable
	{
		CCriticalSection();
		~CCriticalSection();

		void Lock();
		void Unlock();
		bool TryLock();

	private:
		Handle_t Handle;
	};
}
