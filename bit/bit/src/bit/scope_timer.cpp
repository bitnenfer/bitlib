#include <bit/scope_timer.h>
#include <bit/os.h>

bit::ScopeTimer::ScopeTimer(const char* TimerName) :
	TimerName(TimerName)
{
	StartTime = bit::GetSeconds();
}

bit::ScopeTimer::~ScopeTimer()
{
	double EndTime = bit::GetSeconds();
	BIT_ALWAYS_LOG("Timer %s took %.4lf secs.", TimerName, EndTime - StartTime);
}
