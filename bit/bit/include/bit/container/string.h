#pragma once

#include <bit/container/array.h>
#include <bit/utility/hash.h>

#define BIT_SMALL_OPT_STRING_INLINE_SIZE 16

namespace bit
{
	typedef char CharType_t;

	BITLIB_API_TEMPLATE_STRUCT bit::SmallBlockStorage<CharType_t, BIT_SMALL_OPT_STRING_INLINE_SIZE>;
	BITLIB_API_TEMPLATE_STRUCT bit::Array<CharType_t, bit::SmallBlockStorage<CharType_t, BIT_SMALL_OPT_STRING_INLINE_SIZE>>;

	typedef bit::Array<
		CharType_t,
		bit::SmallBlockStorage<CharType_t, BIT_SMALL_OPT_STRING_INLINE_SIZE>
	> StringStorage_t;

	/* ASCII String. Maybe at some point use unicode (utf-8 encoding) */
	struct BITLIB_API String
	{
		String();
		String(const CharType_t* RawStr);
		String(const CharType_t* RawStr, SizeType_t Len);
		String(const String& Other);
		String(String&& Other) noexcept;
		String& operator=(const CharType_t* RawStr);
		String& operator=(const String& Other);
		String& operator=(String&& Other) noexcept;
		String& operator+=(const CharType_t* RawStr);
		String& operator+=(const String& Other);
		const CharType_t* operator*() const;
		SizeType_t GetLength() const;
		void Copy(const CharType_t* RawStr, SizeType_t Len);
		void Copy(const String& Other);
		void Append(const CharType_t* RawStr, SizeType_t Len);
		void Append(const String& Other);
		StringStorage_t& GetStorage();

		BITLIB_API friend String operator+(const String& LHS, const String& RHS) { return (bit::String(LHS) += RHS); }
		BITLIB_API friend String operator+(const String& LHS, const CharType_t* RHS) { return (bit::String(LHS) += RHS); }
		BITLIB_API friend String operator+(const CharType_t* LHS, const String& RHS) { return (bit::String(LHS) += RHS); }
		BITLIB_API friend bool operator==(const String& LHS, const String& RHS);
		BITLIB_API friend bool operator!=(const String& LHS, const String& RHS);

		static String Format(const CharType_t* Fmt, ...);

	private:
		StringStorage_t Storage;
	};

	template<>
	struct BITLIB_API Hash<String>
	{
		typedef size_t HashType_t;
		HashType_t operator()(const String& String) const
		{
			return bit::MurmurHash(*String, String.GetLength(), bit::DEFAULT_HASH_SEED);
		}
	};

	template<>
	struct BITLIB_API Hash<const char*>
	{
		typedef size_t HashType_t;
		HashType_t operator()(const char* Str) const
		{
			return bit::MurmurHash(Str, bit::Strlen(Str), bit::DEFAULT_HASH_SEED);
		}
	};

	BITLIB_API String FormatSize(size_t Size);
}
