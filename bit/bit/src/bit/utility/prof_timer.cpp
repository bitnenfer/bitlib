#include <bit/utility/prof_timer.h>
#include <bit/core/os/debug.h>

bit::ScopeProfTimer::ScopeProfTimer(const char* TimerName) :
	TimerName(TimerName)
{
	Timer.Begin();
}

bit::ScopeProfTimer::~ScopeProfTimer()
{
	double Total = Timer.End();
	BIT_ALWAYS_LOG("Timer %s took %.4lf secs.", TimerName, Total);
}

void bit::ProfTimer::Begin()
{
	StartTime = bit::GetSeconds();
}

double bit::ProfTimer::End()
{
	return bit::GetSeconds() - StartTime;
}
