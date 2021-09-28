#pragma once

#include <bit/core/types.h>

namespace bit
{
	struct BITLIB_API ProfTimer
	{
		void Begin();
		double End();

	private:
		double StartTime = 0.0;
	};

	struct BITLIB_API ScopeProfTimer
	{
		ScopeProfTimer(const char* TimerName);
		~ScopeProfTimer();

	private:
		const char* TimerName;
		ProfTimer Timer;
	};
}
