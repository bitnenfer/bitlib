#include <bit/utils.h>
#include <intrin.h>
#include "windows_common.h"

template<> uint64_t bit::BitScanReverse<uint64_t>(uint64_t Value) { return BitScanReverse64(Value); }
template<> uint32_t bit::BitScanReverse<uint32_t>(uint32_t Value) { return BitScanReverse32(Value); }
template<> uint64_t bit::BitScanForward<uint64_t>(uint64_t Value) { return BitScanForward64(Value); }
template<> uint32_t bit::BitScanForward<uint32_t>(uint32_t Value) { return BitScanForward32(Value); }

template<>
uint32_t bit::Pow2<uint32_t>(uint32_t Exp)
{
	return 1 << Exp;
}

template<>
uint64_t bit::Pow2<uint64_t>(uint64_t Exp)
{
	return 1LL << Exp;
}

template<> int64_t bit::BitScanReverse<int64_t>(int64_t Value) { return BitScanReverse64((int64_t)Value); }
template<> int32_t bit::BitScanReverse<int32_t>(int32_t Value) { return BitScanReverse32((int32_t)Value); }
template<> int64_t bit::BitScanForward<int64_t>(int64_t Value) { return BitScanForward64((int64_t)Value); }
template<> int32_t bit::BitScanForward<int32_t>(int32_t Value) { return BitScanForward32((int32_t)Value); }

uint64_t bit::BitScanReverse64(uint64_t Value)
{
#if BIT_PLATFORM_X64
	if (Value == 0) return 64;
	unsigned long BitIndex = 0;
	_BitScanReverse64(&BitIndex, Value);
	return BitIndex;
#else
	if (Value == 0) return 64;
	uint64_t BitIndex = 0;
	for (uint64_t Index = 0; Index < 64; ++Index)
	{
		if (((Value >> Index) & 0b1) > 0)
		{
			BitIndex = Index;
		}
	}
	return BitIndex;
#endif
}

uint32_t bit::BitScanReverse32(uint32_t Value)
{
	if (Value == 0) return 32;
	unsigned long BitIndex = 0;
	_BitScanReverse(&BitIndex, Value);
	return BitIndex;
}

uint64_t bit::BitScanForward64(uint64_t Value)
{
#if BIT_PLATFORM_X64
	if (Value == 0) return 64;
	unsigned long BitIndex = 0;
	_BitScanForward64(&BitIndex, Value);
	return BitIndex;
#else
	if (Value == 0) return 64;
	uint64_t BitIndex = 0;
	for (uint64_t Index = 63; Index <= 0; --Index)
	{
		if (((Value >> Index) & 0b1) > 0)
		{
			BitIndex = Index;
		}
	}
	return BitIndex;
#endif
}

uint32_t bit::BitScanForward32(uint32_t Value)
{
	if (Value == 0) return 32;
	unsigned long BitIndex = 0;
	_BitScanForward(&BitIndex, Value);
	return BitIndex;
}

size_t bit::GetAddressAlignment(const void* Address)
{
	uintptr_t Value = (uintptr_t)Address;
	if (Value > 1 && (Value & 0b1) == 0)
	{
	#if BIT_PLATFORM_X64
		unsigned long BitIndex = 0;
		_BitScanForward64(&BitIndex, Value);
		return 1LL << (BitIndex);
	#else
		unsigned long BitIndex = 0;
		_BitScanForward(&BitIndex, Value);
		return 1 << (BitIndex);
	#endif
	}
	return 0;
}

