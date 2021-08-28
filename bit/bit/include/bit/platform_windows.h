#pragma once

#define BIT_PLATFORM_WINDOWS 1

#if defined(_WIN64) && _WIN64
	#define BIT_PLATFORM_X64 1
	#define BIT_PLATFORM_X86 0
#elif defined(_WIN32) && _WIN32
	#define BIT_PLATFORM_X64 0
	#define BIT_PLATFORM_X86 1
#endif

#define BIT_DEBUG_BREAK() __debugbreak()

#if _DEBUG
#define BIT_BUILD_DEBUG 1
#define BIT_BUILD_RELEASE 0
#else
#define BIT_BUILD_DEBUG 0
#define BIT_BUILD_RELEASE 1
#endif

#ifndef BIT_STATIC_LIB
#ifdef BIT_EXPORTING
#define BIT_API __declspec(dllexport)
#else
#define BIT_API __declspec(dllimport)
#endif
#else
#define BIT_API
#endif

#define BIT_FORCEINLINE __forceinline
#define BIT_FORCENOINLINE __declspec(noinline)
#define BIT_ALIGN(N) __declspec(align(N))
#define BIT_RESTRICT __declspec(restrict)
#define BIT_DEPRECATED(Info) __declspec(deprecated(Info))
#define BIT_ALLOCATOR __declspec(allocator)
