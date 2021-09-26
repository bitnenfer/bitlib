#pragma once

#include <bit/utils.h>

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
