#include <bit/os.h>
#include <bit/memory.h>
#include <stdio.h>
#include <stdarg.h>
#include "windows_common.h"

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
	OutputDebugStringA(TempFmtString("%s\n", CurrBuffer));
	printf("%s", CurrBuffer);
	MessageBoxA(nullptr, CurrBuffer, "Alert", MB_OK | MB_ICONWARNING);
	Index = (Index + 1) % 3;
	va_end(VaArgs);
}

void bit::ExitProgram(int32_t ExitCode)
{
	ExitProcess(ExitCode);
}
