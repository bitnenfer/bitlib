#include <bit/core/os/debug.h>
#include <bit/core/os/os.h>
#include <bit/core/memory.h>
#include <stdio.h>
#include <stdarg.h>
#include "../../windows_common.h"

int64_t GTimerFrequency = 0;

static SYSTEM_INFO BitGetSystemInfo()
{
	static SYSTEM_INFO SystemInfo = {};
	static bool bComplete = false;
	if (!bComplete)
	{
		GetSystemInfo(&SystemInfo);
	}
	return SystemInfo;
}

static bit::ProcessorArch BitGetProcArch()
{
	static SYSTEM_INFO SystemInfo = BitGetSystemInfo();
	switch (SystemInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64: return bit::ProcessorArch::PROC_ARCH_X64;
	case PROCESSOR_ARCHITECTURE_ARM: return bit::ProcessorArch::PROC_ARCH_ARM;
	case PROCESSOR_ARCHITECTURE_ARM64: return bit::ProcessorArch::PROC_ARCH_ARM64;
	case PROCESSOR_ARCHITECTURE_IA64: return bit::ProcessorArch::PROC_ARCH_IA64;
	case PROCESSOR_ARCHITECTURE_INTEL: return bit::ProcessorArch::PROC_ARCH_X86;
	}
	return bit::ProcessorArch::PROC_ARCH_UNKNOWN;
}

void BitOSInit()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&GTimerFrequency);
	bit::GetGlobalAllocator();
}

size_t bit::GetOSPageSize()
{
	static SYSTEM_INFO SystemInfo = BitGetSystemInfo();
	return SystemInfo.dwPageSize;
}

size_t bit::GetOSAllocationGranularity()
{
	static SYSTEM_INFO SystemInfo = BitGetSystemInfo();
	return SystemInfo.dwAllocationGranularity;
}

void* bit::GetOSMinAddress()
{
	static SYSTEM_INFO SystemInfo = BitGetSystemInfo();
	return SystemInfo.lpMinimumApplicationAddress;
}

void* bit::GetOSMaxAddress()
{
	static SYSTEM_INFO SystemInfo = BitGetSystemInfo();
	return SystemInfo.lpMaximumApplicationAddress;
}

int32_t bit::GetOSProcessorCount()
{
	static SYSTEM_INFO SystemInfo = BitGetSystemInfo();
	return SystemInfo.dwNumberOfProcessors;
}

bit::ProcessorArch bit::GetOSProcessorArch()
{
	static SYSTEM_INFO SystemInfo = BitGetSystemInfo();
	static bit::ProcessorArch ProcArch = BitGetProcArch();
	return ProcArch;
}

bit::ProcessMemoryInfo bit::GetOSProcessMemoryInfo()
{
	ProcessMemoryInfo Info = {};
	Info.PageSize = GetOSPageSize();
	Info.AllocationGranularity = GetOSAllocationGranularity();

	PROCESS_MEMORY_COUNTERS MemCounters = {};
	MemCounters.cb = sizeof(MemCounters);
	if (GetProcessMemoryInfo(GetCurrentProcess(), &MemCounters, sizeof(MemCounters)) == TRUE)
	{
		Info.PhysicalUsedInBytes = MemCounters.WorkingSetSize;
		Info.PhysicalPeakUsedInBytes = MemCounters.PeakWorkingSetSize;
		Info.VirtualUsedInBytes = MemCounters.PagefileUsage;
		Info.VirtualPeakUsedInBytes = MemCounters.PeakPagefileUsage;
	}

	MEMORYSTATUSEX MemStat = {};
	MemStat.dwLength = sizeof(MemStat);
	if (GlobalMemoryStatusEx(&MemStat) == TRUE)
	{
		Info.PhysicalTotalInBytes = MemStat.ullTotalPhys;
		Info.VirtualTotalInBytes = MemStat.ullTotalVirtual;
	}

	return Info;
}


void bit::OutputLog(const char* Fmt, ...)
{
	static char Buffer[4096 * 3] = {};
	static uint32_t Index = 0;
	char* CurrBuffer = &Buffer[Index * 4096];
	va_list VaArgs;
	va_start(VaArgs, Fmt);
	vsprintf_s(CurrBuffer, 4096, Fmt, VaArgs);
	OutputDebugStringA(CurrBuffer);
	printf("%s", CurrBuffer);
	va_end(VaArgs);
	Index = (Index + 1) % 3;
}

void bit::Alert(const char* Fmt, ...)
{
	static char Buffer[4096 * 3] = {};
	static uint32_t Index = 0;
	char* CurrBuffer = &Buffer[Index * 4096];
	va_list VaArgs;
	va_start(VaArgs, Fmt);
	vsprintf_s(CurrBuffer, 4096, Fmt, VaArgs);
	OutputDebugStringA(TempFmtString("%s", CurrBuffer));
	printf("%s", CurrBuffer);
	MessageBoxA(nullptr, CurrBuffer, "Alert", MB_OK | MB_ICONWARNING);
	Index = (Index + 1) % 3;
	va_end(VaArgs);
}

void bit::ExitProgram(int32_t ExitCode)
{
	ExitProcess(ExitCode);
}

double bit::GetSeconds()
{
	int64_t Time;
	QueryPerformanceCounter((LARGE_INTEGER*)&Time);
	return (double)(Time * 1000 / GTimerFrequency) / 1000.0;
}

int32_t bit::GetOSErrorCode()
{
	return (int32_t)GetLastError();
}