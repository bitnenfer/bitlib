#include <bit/utils.h>
#include <intrin.h>

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
