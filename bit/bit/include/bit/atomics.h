#pragma once

#include <bit/types.h>

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

	BITLIB_API uint32_t AtomicExchange(uint32_t* Target, uint32_t Value);
	BITLIB_API uint32_t AtomicCompareExchange(uint32_t* Target, uint32_t Value, uint32_t Comperand);
	BITLIB_API uint32_t AtomicAdd(uint32_t* Target, uint32_t Value);
	BITLIB_API uint32_t AtomicSubtract(uint32_t* Target, uint32_t Value);
	BITLIB_API uint32_t AtomicIncrement(uint32_t* Target);
	BITLIB_API uint32_t AtomicDecrement(uint32_t* Target);
	BITLIB_API uint32_t AtomicPostIncrement(uint32_t* Target);
	BITLIB_API uint32_t AtomicPostDecrement(uint32_t* Target);
}
