#pragma once

#include <bit/array.h>
#include <bit/hash.h>

#define BIT_TXT(str) (str)
#define BIT_SMALL_OPT_STRING_INLINE_SIZE 8

namespace bit
{
	typedef char CharType_t;

	BITLIB_API_TEMPLATE_STRUCT bit::TInlineBlockAllocator<CharType_t, BIT_SMALL_OPT_STRING_INLINE_SIZE>;
	BITLIB_API_TEMPLATE_STRUCT bit::TArray<CharType_t, bit::TInlineBlockAllocator<CharType_t, BIT_SMALL_OPT_STRING_INLINE_SIZE>>;

	typedef bit::TArray<
		CharType_t,
		bit::TInlineBlockAllocator<CharType_t, BIT_SMALL_OPT_STRING_INLINE_SIZE>
	> StringStorage_t;

	/* ASCII String. Maybe at some point use unicode (utf-8 encoding) */
	struct BITLIB_API CString
	{
		CString();
		CString(const CharType_t* RawStr);
		CString(const CharType_t* RawStr, SizeType_t Len);
		CString(const CString& Other);
		CString(CString&& Other) noexcept;
		CString& operator=(const CharType_t* RawStr);
		CString& operator=(const CString& Other);
		CString& operator=(CString&& Other) noexcept;
		CString& operator+=(const CharType_t* RawStr);
		CString& operator+=(const CString& Other);
		const CharType_t* operator*() const;
		SizeType_t GetLength() const;
		void Copy(const CharType_t* RawStr, SizeType_t Len);
		void Copy(const CString& Other);
		void Append(const CharType_t* RawStr, SizeType_t Len);
		void Append(const CString& Other);
		StringStorage_t& GetStorage();

		BITLIB_API friend CString operator+(const CString& LHS, const CString& RHS) { return (bit::CString(LHS) += RHS); }
		BITLIB_API friend CString operator+(const CString& LHS, const CharType_t* RHS) { return (bit::CString(LHS) += RHS); }
		BITLIB_API friend CString operator+(const CharType_t* LHS, const CString& RHS) { return (bit::CString(LHS) += RHS); }
		BITLIB_API friend bool operator==(const CString& LHS, const CString& RHS);
		BITLIB_API friend bool operator!=(const CString& LHS, const CString& RHS);

		static CString Format(const CharType_t* Fmt, ...);

	private:
		StringStorage_t Storage;
	};

	template<>
	struct BITLIB_API THash<CString>
	{
		typedef size_t HashType_t;
		HashType_t operator()(const CString& String) const
		{
			return bit::MurmurHash(*String, String.GetLength(), bit::DEFAULT_HASH_SEED);
		}
	};
}
