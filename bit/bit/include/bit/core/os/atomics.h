#pragma once

#include <bit/core/types.h>

namespace bit
{
	BITLIB_API int64_t AtomicExchange(int64_t* Target, int64_t Value);
	BITLIB_API int64_t AtomicCompareExchange(int64_t* Target, int64_t Value, int64_t Comperand);
	BITLIB_API int64_t AtomicAdd(int64_t* Target, int64_t Value);
	BITLIB_API int64_t AtomicSubtract(int64_t* Target, int64_t Value);
	BITLIB_API int64_t AtomicIncrement(int64_t* Target);
	BITLIB_API int64_t AtomicDecrement(int64_t* Target);
	BITLIB_API int64_t AtomicPostIncrement(int64_t* Target);
	BITLIB_API int64_t AtomicPostDecrement(int64_t* Target);

	BITLIB_API int32_t AtomicExchange(int32_t* Target, int32_t Value);
	BITLIB_API int32_t AtomicCompareExchange(int32_t* Target, int32_t Value, int32_t Comperand);
	BITLIB_API int32_t AtomicAdd(int32_t* Target, int32_t Value);
	BITLIB_API int32_t AtomicSubtract(int32_t* Target, int32_t Value);
	BITLIB_API int32_t AtomicIncrement(int32_t* Target);
	BITLIB_API int32_t AtomicDecrement(int32_t* Target);
	BITLIB_API int32_t AtomicPostIncrement(int32_t* Target);
	BITLIB_API int32_t AtomicPostDecrement(int32_t* Target);
}
