#pragma once

#include <bit/core/types.h>

#define BIT_UNUSED_VAR(n) ((void)&(n))

namespace bit
{
	struct BITLIB_API NonCopyable
	{
	public:
		NonCopyable() {}

	private:
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};

	struct BITLIB_API NonMovable
	{
	public:
		NonMovable() {}

	private:
		NonMovable(NonMovable&&) = delete;
		NonMovable& operator=(NonMovable&&) = delete;
	};

	template<typename T, T TValue> struct ConstValue { static constexpr T Value = TValue; };

	template<typename T> struct IsSigned : public ConstValue<bool, false> {};
	template<> struct IsSigned<int8_t> : public ConstValue<bool, true> {};
	template<> struct IsSigned<int16_t> : public ConstValue<bool, true> {};
	template<> struct IsSigned<int32_t> : public ConstValue<bool, true> {};
	template<> struct IsSigned<int64_t> : public ConstValue<bool, true> {};

	template<typename T> struct RemoveRef { typedef T Type; };
	template<typename T> struct RemoveRef<T&> { typedef T Type; };
	template<typename T> struct RemoveRef<T&&> { typedef T Type; };

	template<typename T> struct IsLValueRef : public ConstValue<bool, false> {};
	template<typename T> struct IsLValueRef<T&> : public ConstValue<bool, true> {};

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

	template<class T>
	constexpr T ConstMax(T A, T B)
	{
		if (A > B) return A;
		return B;
	}
	template<class T>
	constexpr T ConstMin(T A, T B)
	{
		if (A < B) return A;
		return B;
	}

	template<typename T>
	constexpr T ConstClamp(T Value, T MinValue, T MaxValue)
	{
		return (Value >= MaxValue) ? MaxValue : ((Value <= MinValue) ? MinValue : Value);
	}

	template<typename TLHS, typename TRHS>
	BIT_FORCEINLINE TLHS BitCast(TRHS Value) 
	{ 
		return *(TLHS*)&Value; 
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
		#elif BIT_PLATFORM_X86
		return ((Value)+((Alignment)-1)) & ~((Alignment)-1);
		#endif
	}

	BIT_FORCEINLINE void* AlignPtr(void* Ptr, size_t Alignment)
	{
	#if BIT_PLATFORM_X64
		return (void*)(((uintptr_t)(Ptr)+((uintptr_t)(Alignment)-1LL)) & ~((uintptr_t)(Alignment)-1LL));
	#elif BIT_PLATFORM_X86
		return (void*)(((uintptr_t)(Ptr)+((uintptr_t)(Alignment)-1)) & ~((uintptr_t)(Alignment)-1));
	#endif
	}

	template<typename T = void>
	T* OffsetPtr(void* Ptr, intptr_t Offset)
	{
		return reinterpret_cast<T*>((intptr_t)Ptr + Offset);
	}

	BIT_FORCEINLINE size_t PtrDiff(const void* A, const void* B)
	{
		return (size_t)bit::Abs((intptr_t)A - (intptr_t)B);
	}

	BITLIB_API uint64_t BitScanReverse64(uint64_t Value);
	BITLIB_API uint32_t BitScanReverse32(uint32_t Value);
	BITLIB_API uint64_t BitScanForward64(uint64_t Value);
	BITLIB_API uint32_t BitScanForward32(uint32_t Value);

	template<typename T>
	T BitScanReverse(T Value);
	template<> BITLIB_API uint64_t BitScanReverse<uint64_t>(uint64_t Value);
	template<> BITLIB_API uint32_t BitScanReverse<uint32_t>(uint32_t Value);
	template<> BITLIB_API uint16_t BitScanReverse<uint16_t>(uint16_t Value);
	template<> BITLIB_API int64_t BitScanReverse<int64_t>(int64_t Value);
	template<> BITLIB_API int32_t BitScanReverse<int32_t>(int32_t Value);

	template<typename T>
	T BitScanForward(T Value);
	template<> BITLIB_API uint64_t BitScanForward<uint64_t>(uint64_t Value);
	template<> BITLIB_API uint32_t BitScanForward<uint32_t>(uint32_t Value);
	template<> BITLIB_API uint16_t BitScanForward<uint16_t>(uint16_t Value);

	constexpr uint64_t ConstBitScanReverse64(uint64_t Value)
	{
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
	}
	constexpr uint32_t ConstBitScanReverse32(uint32_t Value)
	{
		if (Value == 0) return 32;
		uint32_t BitIndex = 0;
		for (uint32_t Index = 0; Index < 32; ++Index)
		{
			if (((Value >> Index) & 0b1) > 0)
			{
				BitIndex = Index;
			}
		}
		return BitIndex;
	}

	template<typename T>
	constexpr T ConstBitScanReverse(T Value);
	template<> constexpr uint64_t ConstBitScanReverse<uint64_t>(uint64_t Value) { return ConstBitScanReverse64(Value); }
	template<> constexpr uint32_t ConstBitScanReverse<uint32_t>(uint32_t Value) { return ConstBitScanReverse32(Value); }
	template<> constexpr uint16_t ConstBitScanReverse<uint16_t>(uint16_t Value) { return ConstBitScanReverse32((uint32_t)Value); }

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
		size_t Exp = BitScanReverse(Size - 1) + 1;
	#if BIT_PLATFORM_X64
		return 1LL << Exp;
	#elif BIT_PLATFORM_X86
		return 1 << Exp;
	#endif
	}

	constexpr size_t ConstNextPow2(size_t Size)
	{
		size_t Exp = ConstBitScanReverse(Size - 1) + 1;
	#if BIT_PLATFORM_X64
		return 1LL << Exp;
	#elif BIT_PLATFORM_X86
		return 1 << Exp;
	#endif
	}

	template<typename T> 
	BIT_FORCEINLINE bool IsPow2(T Value)
	{
		// Value		= 0b001000
		// Value - 1	= 0b000111
		// Doing a bitwise AND should result in 0
		return (((Value) & ((Value)-1)) == 0);
	}

	template<typename T>
	T Pow2(T Exp);

	template<>
	BITLIB_API uint64_t Pow2<uint64_t>(uint64_t Exp);

	template<>
	BITLIB_API uint32_t Pow2<uint32_t>(uint32_t Exp);

	template<>
	BITLIB_API uint16_t Pow2<uint16_t>(uint16_t Exp);

	BIT_FORCEINLINE bool PtrInRange(const void* Ptr, const void* Start, const void* End)
	{
		return Ptr >= Start && Ptr < End;
	}

	BIT_FORCEINLINE size_t RoundUp(size_t Value, size_t RoundTo)
	{
		return ((Value + RoundTo - 1) / RoundTo) * RoundTo;
	}
}
