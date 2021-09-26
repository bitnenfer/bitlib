#pragma once

#include <bit/core/types.h>
#include <bit/utility/utility.h>

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
