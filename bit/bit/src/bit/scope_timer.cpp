#include <bit/scope_timer.h>
#include <bit/os.h>

bit::CScopeTimer::CScopeTimer(const char* TimerName) :
	TimerName(TimerName)
{
	StartTime = bit::GetSeconds();
}

bit::CScopeTimer::~CScopeTimer()
{
	double EndTime = bit::GetSeconds();
	BIT_LOG("Timer %s took %.4lf secs.\n", TimerName, EndTime - StartTime);
}
