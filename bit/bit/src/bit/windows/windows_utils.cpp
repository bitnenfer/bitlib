#include <bit/utils.h>
#include <intrin.h>
#include "windows_common.h"

size_t bit::Log2(size_t Value)
{
	#if BIT_PLATFORM_X64
	unsigned long BitIndex = 0;
	_BitScanReverse64(&BitIndex, Value);
	return BitIndex;
	#else
	unsigned long BitIndex = 0;
	_BitScanReverse(&BitIndex, Value);
	return BitIndex;
	#endif
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
