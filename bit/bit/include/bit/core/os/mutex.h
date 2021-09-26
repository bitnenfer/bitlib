#pragma once

#include <bit/utility/utility.h>

namespace bit
{
	struct BITLIB_API Mutex : public NonCopyable
	{
		Mutex();
		~Mutex();

		void Lock();
		void Unlock();

	private:
		Handle_t Handle;
	};
}
