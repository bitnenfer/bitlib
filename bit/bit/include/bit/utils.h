#pragma once

#include <bit/types.h>

#define BIT_UNUSED_VAR(n) ((void)&(n))

namespace bit
{
	struct BITLIB_API CNonCopyable
	{
	public:
		CNonCopyable() {}

	private:
		CNonCopyable(const CNonCopyable&) = delete;
		CNonCopyable& operator=(const CNonCopyable&) = delete;
	};

	template<typename T, T TValue> struct TConstValue { static BIT_CONSTEXPR T Value = TValue; };

	template<typename T> struct TIsSigned : public TConstValue<bool, false> {};
	template<> struct TIsSigned<int8_t> : public TConstValue<bool, true> {};
	template<> struct TIsSigned<int16_t> : public TConstValue<bool, true> {};
	template<> struct TIsSigned<int32_t> : public TConstValue<bool, true> {};
	template<> struct TIsSigned<int64_t> : public TConstValue<bool, true> {};

	template<typename T> struct RemoveRef { typedef T Type; };
	template<typename T> struct RemoveRef<T&> { typedef T Type; };
	template<typename T> struct RemoveRef<T&&> { typedef T Type; };

	template<typename T> struct TIsLValueRef : public TConstValue<bool, false> {};
	template<typename T> struct TIsLValueRef<T&> : public TConstValue<bool, true> {};

	template <typename T>
	typename RemoveRef<T>::Type&& Move(T&& Arg) noexcept
	{
		return static_cast<typename RemoveRef<T>::Type&&>(Arg);
	}
	
	template<typename T>
	T&& Forward(typename bit::RemoveRef<T>::Type& Arg) noexcept
	{
		return static_cast<T&&>(Arg);
	}

	template<typename T>
	T&& Forward(typename bit::RemoveRef<T>::Type&& Arg) noexcept
	{
		return static_cast<T&&>(Arg);
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

	template<typename T> 
	BIT_FORCEINLINE T Clamp(T Value, T MinValue, T MaxValue) 
	{ 
		return (Value >= MaxValue) ? MaxValue : ((Value <= MinValue) ? MinValue : Value); 
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

	// This function counts the number of bits before a set bit is found
	// and assumes the alignment based on that. It might not be the selected alignment
	// at allocation time, but it'll always be greater or equal.
	BITLIB_API size_t GetAddressAlignment(const void* Address);
	BIT_FORCEINLINE bool IsAddressAligned(const void* Address, size_t Alignment)
	{
		return (Alignment > 1) ? ((uintptr_t)Address % Alignment) == 0 : true;
	}

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

	BIT_FORCEINLINE size_t Pow2(size_t Exp)
	{
	#if BIT_PLATFORM_X64
		return 1LL << Exp;
	#elif BIT_PLATFORM_X86
		return 1 << Exp;
	#endif
	}

	BIT_FORCEINLINE bool PtrInRange(const void* Ptr, const void* Start, const void* End)
	{
		return Ptr >= Start && Ptr <= End;
	}

}
