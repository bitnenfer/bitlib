#pragma once

#include <bit/types.h>

namespace bit
{
	struct BITLIB_API ScopeTimer
	{
		ScopeTimer(const char* TimerName);
		~ScopeTimer();

	private:
		const char* TimerName;
		double StartTime;
	};
}
