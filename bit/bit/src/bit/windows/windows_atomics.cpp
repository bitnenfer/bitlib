#include <bit/atomics.h>
#include "windows_common.h"

int64_t bit::AtomicExchange(int64_t* Target, int64_t Value)
{
    return InterlockedExchange64(Target, Value);
}

int64_t bit::AtomicCompareExchange(int64_t* Target, int64_t Value, int64_t Comperand)
{
    return InterlockedCompareExchange64(Target, Value, Comperand);
}

int64_t bit::AtomicAdd(int64_t* Target, int64_t Value)
{
    return InterlockedExchangeAdd64(Target, Value);
}

int64_t bit::AtomicSubtract(int64_t* Target, int64_t Value)
{
    return InterlockedExchangeAdd64(Target, -Value);
}

int64_t bit::AtomicIncrement(int64_t* Target)
{
    return InterlockedIncrement64(Target);
}

int64_t bit::AtomicDecrement(int64_t* Target)
{
    return InterlockedDecrement64(Target);
}

int64_t bit::AtomicPostIncrement(int64_t* Target)
{
    return InterlockedExchangeAdd64(Target, 1);
}

int64_t bit::AtomicPostDecrement(int64_t* Target)
{
    return InterlockedExchangeAdd64(Target, -1);
}

int32_t bit::AtomicExchange(int32_t* Target, int32_t Value)
{
    return InterlockedExchange((LONG*)Target, Value);
}

int32_t bit::AtomicCompareExchange(int32_t* Target, int32_t Value, int32_t Comperand)
{
    return InterlockedCompareExchange((LONG*)Target, Value, Comperand);
}

int32_t bit::AtomicAdd(int32_t* Target, int32_t Value)
{
    return InterlockedExchangeAdd((LONG*)Target, Value);
}

int32_t bit::AtomicSubtract(int32_t* Target, int32_t Value)
{
    return InterlockedExchangeAdd((LONG*)Target, -Value);
}

int32_t bit::AtomicIncrement(int32_t* Target)
{
    return InterlockedIncrement((LONG*)Target);
}

int32_t bit::AtomicDecrement(int32_t* Target)
{
    return InterlockedDecrement((LONG*)Target);
}

int32_t bit::AtomicPostIncrement(int32_t* Target)
{
    return InterlockedExchangeAdd((LONG*)Target, 1);
}

int32_t bit::AtomicPostDecrement(int32_t* Target)
{
    return InterlockedExchangeAdd((LONG*)Target, -1);
}
