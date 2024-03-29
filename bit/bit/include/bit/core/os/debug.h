#pragma once

#include <bit/core/types.h>
#include <bit/core/os/os.h>

namespace bit
{
	BITLIB_API void OutputLog(const char* Fmt, ...);
	BITLIB_API void Alert(const char* Fmt, ...);
}

#if BIT_BUILD_DEBUG
#define BIT_ALERT(Fmt, ...) bit::Alert(Fmt "\n", ##__VA_ARGS__); BIT_DEBUG_BREAK();
#define BIT_ASSERT_MSG(Condition, Fmt, ...) if (!(Condition)) { BIT_ALERT(Fmt "\n", ##__VA_ARGS__); BIT_DEBUG_BREAK(); }
#define BIT_LOG(Fmt, ...) bit::OutputLog(Fmt "\n", ##__VA_ARGS__)
#define BIT_PANIC_MSG(Fmt, ...) { BIT_ALERT(Fmt "\n", ##__VA_ARGS__);  bit::ExitProgram(-1); }
#define BIT_PANIC() { BIT_DEBUG_BREAK();  bit::ExitProgram(-1); }
#define BIT_ASSERT(Condition) if (!(Condition)) { BIT_DEBUG_BREAK(); }
#else
#define BIT_ALERT(Fmt, ...)
#define BIT_ASSERT_MSG(Condition, Fmt, ...) (Condition)
#define BIT_LOG(Fmt, ...) 
#define BIT_PANIC_MSG(Fmt, ...) 
#define BIT_PANIC() 
#define BIT_ASSERT(Condition) (Condition)
#endif

#define BIT_ALWAYS_ALERT(Fmt, ...) bit::Alert(Fmt "\n", ##__VA_ARGS__); BIT_DEBUG_BREAK();
#define BIT_ALWAYS_LOG(Fmt, ...) bit::OutputLog(Fmt "\n", ##__VA_ARGS__)
#define BIT_ALWAYS_PANIC()  { BIT_DEBUG_BREAK();  bit::ExitProgram(-1); }
#define BIT_ALWAYS_PANIC_MSG(Fmt, ...) { BIT_ALWAYS_ALERT(Fmt "\n", ##__VA_ARGS__);  bit::ExitProgram(-1); }
#define BIT_ALWAYS_ASSERT(Condition) if (!(Condition)) { BIT_DEBUG_BREAK(); }
#define BIT_ALWAYS_ASSERT_MSG(Condition, Fmt, ...) if (!(Condition)) { BIT_ALWAYS_ALERT(Fmt, ##__VA_ARGS__); BIT_DEBUG_BREAK(); }
