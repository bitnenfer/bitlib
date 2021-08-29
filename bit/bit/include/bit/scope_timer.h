#pragma once

#include <bit/types.h>

namespace bit
{
	struct BITLIB_API CScopeTimer
	{
		CScopeTimer(const char* TimerName);
		~CScopeTimer();

	private:
		const char* TimerName;
		double StartTime;
	};
}
