#pragma once

#define BIT_PLATFORM_WINDOWS 1

#if defined(_WIN64) && _WIN64
#define BIT_PLATFORM_X64 1
#define BIT_PLATFORM_X86 0
#define BIT_INVALID_ADDRESS ((void*)0xDEADBEEFDEADBEEF)
#elif defined(_WIN32) && _WIN32
#define BIT_PLATFORM_X64 0
#define BIT_PLATFORM_X86 1
#define BIT_INVALID_ADDRESS ((void*)0xDEADBEEF)
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
#define BITLIB_API __declspec(dllexport)
#else
#define BITLIB_API __declspec(dllimport)
#endif
#define BITLIB_API_TEMPLATE_CLASS template class BITLIB_API
#define BITLIB_API_TEMPLATE_STRUCT template struct BITLIB_API
#else
#define BITLIB_API
#define BITLIB_API_TEMPLATE
#endif

#define BIT_FORCEINLINE __forceinline
#define BIT_FORCENOINLINE __declspec(noinline)
#define BIT_ALIGN(N) __declspec(align(N))
#define BIT_RESTRICT __declspec(restrict)
#define BIT_DEPRECATED(Info) __declspec(deprecated(Info))
#define BIT_ALLOCATOR __declspec(allocator)

#if defined(__cplusplus) && __cplusplus >= 201103L
#define BIT_CPP_VER __cplusplus
#define BIT_CPP17 201703L
#define BIT_CPP14 201402L
#define BIT_CPP11 201103L
#else
#error "This is a C++ library. Requires > C++11. If you're using MSVC, set this compiler option /Zc:__cplusplus"
#endif

#if BIT_CPP_VER >= BIT_CPP17
#define BIT_IF_CONSTEXPR if constexpr
#else
#define BIT_IF_CONSTEXPR if
#endif
