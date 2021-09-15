#pragma once

#include <bit/platform.h>

#ifndef _STDINT
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#if BIT_PLATFORM_X86
typedef unsigned int		uintptr_t;
typedef unsigned int		size_t;
typedef int					ptrdiff_t;
typedef int					intptr_t;
typedef uint32_t			usize_t;
typedef int32_t				ssize_t;
#elif BIT_PLATFORM_X64
typedef unsigned __int64	uintptr_t;
typedef unsigned __int64	size_t;
typedef unsigned __int64	usize_t;
typedef __int64	intptr_t;
typedef __int64				ptrdiff_t;
typedef uint64_t			usize_t;
typedef int64_t				ssize_t;
#endif
#endif

namespace bit
{
	typedef void* Handle_t;
}

