#pragma once

#include <bit/utils.h>

namespace bit
{
	struct BITLIB_API CMutex : public CNonCopyable
	{
		CMutex();
		~CMutex();

		void Lock();
		void Unlock();

	private:
		Handle_t Handle;
	};
}
