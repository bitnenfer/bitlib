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

uint32_t bit::AtomicExchange(uint32_t* Target, uint32_t Value)
{
    return InterlockedExchange(Target, Value);
}

uint32_t bit::AtomicCompareExchange(uint32_t* Target, uint32_t Value, uint32_t Comperand)
{
    return InterlockedCompareExchange(Target, Value, Comperand);
}

uint32_t bit::AtomicAdd(uint32_t* Target, uint32_t Value)
{
    return InterlockedExchangeAdd(Target, Value);
}

uint32_t bit::AtomicSubtract(uint32_t* Target, uint32_t Value)
{
    return InterlockedExchangeSubtract(Target, Value);
}

uint32_t bit::AtomicIncrement(uint32_t* Target)
{
    return InterlockedIncrement(Target);
}

uint32_t bit::AtomicDecrement(uint32_t* Target)
{
    return InterlockedDecrement(Target);
}

uint32_t bit::AtomicPostIncrement(uint32_t* Target)
{
    return InterlockedExchangeAdd(Target, 1);
}

uint32_t bit::AtomicPostDecrement(uint32_t* Target)
{
    return InterlockedExchangeSubtract(Target, -1);
}
