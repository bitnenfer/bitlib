#pragma once

#include <bit/types.h>

namespace bit
{
	template<class T> struct RemoveRef { typedef T Type; };
	template<class T> struct RemoveRef<T&> { typedef T Type; };
	template<class T> struct RemoveRef<T&&> { typedef T Type; };

	template <class T>
	typename RemoveRef<T>::Type&& Move(T&& Arg) noexcept
	{
		return static_cast<typename RemoveRef<T>::Type&&>(Arg);
	}

	template<class T>
	T Max(T A, T B)
	{
		if (A > B) return A;
		return B;
	}
	template<class T>
	T Min(T A, T B)
	{
		if (A < B) return A;
		return B;
	}

	BIT_FORCEINLINE size_t AlignUint(size_t Value, size_t Alignment)
	{
		#if BIT_PLATFORM_X64
		return ((Value)+((Alignment)-1LL)) & ~((Alignment)-1LL);
		#elif BIT_PLAFORM_X86
		return ((Value)+((Alignment)-1)) & ~((Alignment)-1);
		#endif
	}

	size_t BIT_API Log2(size_t Value);

	BIT_FORCEINLINE size_t NextPow2(size_t Size)
	{
		uint64_t BitIndex = Log2(Size - 1);
		return 1LL << (BitIndex + 1);
	}

	template<typename T> bool IsPow2(T Value)
	{
		// Value		= 0b001000
		// Value - 1	= 0b000111
		// Doing a bitwise AND should result in 0
		return (((Value) & ((Value)-1)) == 0);
	}

	
}
