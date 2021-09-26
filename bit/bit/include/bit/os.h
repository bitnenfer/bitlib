#pragma once

#include <bit/types.h>

namespace bit
{
	enum class ProcessorArch
	{
		PROC_ARCH_UNKNOWN,
		PROC_ARCH_X64,
		PROC_ARCH_ARM,
		PROC_ARCH_ARM64,
		PROC_ARCH_IA64,
		PROC_ARCH_X86
	};

	BITLIB_API void OutputLog(const char* Fmt, ...);
	BITLIB_API void Alert(const char* Fmt, ...);
	BITLIB_API void ExitProgram(int32_t ExitCode);
	BITLIB_API double GetSeconds();
	BITLIB_API int32_t GetOSErrorCode();
	BITLIB_API size_t GetOSPageSize();
	BITLIB_API size_t GetOSAllocationGranularity();
	BITLIB_API void* GetOSMinAddress();
	BITLIB_API void* GetOSMaxAddress();
	BITLIB_API int32_t GetOSProcessorCount();
	BITLIB_API ProcessorArch GetOSProcessorArch();
}

#if BIT_BUILD_DEBUG
#define BIT_ALERT(Fmt, ...) bit::Alert(Fmt "\n", ##__VA_ARGS__); BIT_DEBUG_BREAK();
#define BIT_ASSERT_MSG(Condition, Fmt, ...) if (!(Condition)) { BIT_ALERT(Fmt "\n", ##__VA_ARGS__); BIT_DEBUG_BREAK(); }
#define BIT_LOG(Fmt, ...) bit::OutputLog(Fmt "\n", ##__VA_ARGS__)
#define BIT_PANIC_MSG(Fmt, ...) { BIT_ALERT(Fmt "\n", ##__VA_ARGS__);  bit::ExitProgram(-1); }
#define BIT_PANIC(Fmt) { BIT_DEBUG_BREAK();  bit::ExitProgram(-1); }
#define BIT_ASSERT(Condition) if (!(Condition)) { BIT_DEBUG_BREAK(); }
#else
#define BIT_ALERT(Fmt, ...)
#define BIT_ASSERT_MSG(Condition, Fmt, ...) (Condition)
#define BIT_LOG(Fmt, ...) 
#define BIT_PANIC_MSG(Fmt, ...) 
#define BIT_PANIC(Fmt) 
#define BIT_ASSERT(Condition) (Condition)
#endif

#define BIT_ALWAYS_LOG(Fmt, ...) bit::OutputLog(Fmt "\n", ##__VA_ARGS__)
#define BIT_ALWAYS_PANIC(Fmt)  { BIT_DEBUG_BREAK();  bit::ExitProgram(-1); }
#define BIT_ALWAYS_ASSERT(Condition) if (!(Condition)) { BIT_DEBUG_BREAK(); }
#define BIT_ALWAYS_ASSERT_MSG(Condition, Fmt, ...) if (!(Condition)) { BIT_ALERT(Fmt, ##__VA_ARGS__); BIT_DEBUG_BREAK(); }
