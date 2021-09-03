#pragma once

#include <bit/types.h>

#define BIT_UNUSED_VAR(n) ((void)&(n))

namespace bit
{
	template<class T> struct RemoveRef { typedef T Type; };
	template<class T> struct RemoveRef<T&> { typedef T Type; };
	template<class T> struct RemoveRef<T&&> { typedef T Type; };

	template <class T>
	BIT_FORCEINLINE typename RemoveRef<T>::Type&& Move(T&& Arg) noexcept
	{
		return static_cast<typename RemoveRef<T>::Type&&>(Arg);
	}

	template<class T>
	BIT_FORCEINLINE T Max(T A, T B)
	{
		if (A > B) return A;
		return B;
	}
	template<class T>
	BIT_FORCEINLINE T Min(T A, T B)
	{
		if (A < B) return A;
		return B;
	}

	template<typename TLHS, typename TRHS>
	BIT_FORCEINLINE TLHS BitCast(TRHS Value) 
	{ 
		return *(TLHS*)& Value; 
	}

	template<typename T>
	T Abs(T Value);

	template<typename T = int32_t>
	BIT_FORCEINLINE int32_t Abs(int32_t Value) 
	{ 
		return Value < 0 ? ~Value + 1 : Value; 
	}

	template<typename T = int64_t>
	BIT_FORCEINLINE int64_t Abs(int64_t Value) 
	{ 
		return Value < 0 ? ~Value + 1 : Value; 
	}

	template<typename T = float>
	BIT_FORCEINLINE float Abs(float Value)
	{
		uint32_t Bits = *((uint32_t*)& Value) & 0x7FFFFFFF; // Just clear the sign bit
		return *((float*)& Bits);
	}

	template<typename T = double>
	BIT_FORCEINLINE double Abs(double Value)
	{
		uint64_t Bits = *((uint64_t*)& Value) & 0x7FFFFFFFFFFFFFFFLL; // Just clear the sign bit
		return *((double*)& Bits);
	}

	BIT_FORCEINLINE size_t AlignUint(size_t Value, size_t Alignment)
	{
		#if BIT_PLATFORM_X64
		return ((Value)+((Alignment)-1LL)) & ~((Alignment)-1LL);
		#elif BIT_PLAFORM_X86
		return ((Value)+((Alignment)-1)) & ~((Alignment)-1);
		#endif
	}

	BIT_FORCEINLINE void* AlignPtr(void* Ptr, size_t Alignment)
	{
#if BIT_PLATFORM_X64
		return (void*)(((uintptr_t)(Ptr)+((uintptr_t)(Alignment)-1LL)) & ~((uintptr_t)(Alignment)-1LL));
#elif BIT_PLAFORM_X86
		return (void*)(((uintptr_t)(Ptr)+((uintptr_t)(Alignment)-1)) & ~((uintptr_t)(Alignment)-1));
#endif
	}

	BIT_FORCEINLINE void* ForwardPtr(void* Ptr, size_t Offset)
	{
		return (void*)((uintptr_t)Ptr + (uintptr_t)Offset);
	}

	BIT_FORCEINLINE void* BackwardPtr(void* Ptr, size_t Offset)
	{
		return (void*)((uintptr_t)Ptr - (uintptr_t)Offset);
	}

	BIT_FORCEINLINE size_t PtrDiff(const void* A, const void* B)
	{
		return (size_t)bit::Abs((intptr_t)A - (intptr_t)B);
	}

	BITLIB_API size_t Log2(size_t Value);

	BIT_FORCEINLINE size_t NextPow2(size_t Size)
	{
		uint64_t BitIndex = Log2(Size - 1);
		return 1LL << (BitIndex + 1);
	}

	template<typename T> 
	BIT_FORCEINLINE bool IsPow2(T Value)
	{
		// Value		= 0b001000
		// Value - 1	= 0b000111
		// Doing a bitwise AND should result in 0
		return (((Value) & ((Value)-1)) == 0);
	}

	BIT_FORCEINLINE bool PtrInRange(const void* Ptr, const void* Start, const void* End)
	{
		return Ptr >= Start && Ptr <= End;
	}

	template<typename T> struct TIsSigned { static BIT_CONSTEXPR bool bValue = false; };
	template<> struct TIsSigned<int8_t> { static BIT_CONSTEXPR bool bValue = true; };
	template<> struct TIsSigned<int16_t> { static BIT_CONSTEXPR bool bValue = true; };
	template<> struct TIsSigned<int32_t> { static BIT_CONSTEXPR bool bValue = true; };
	template<> struct TIsSigned<int64_t> { static BIT_CONSTEXPR bool bValue = true; };

}
