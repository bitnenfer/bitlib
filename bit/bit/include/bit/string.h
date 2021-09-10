#pragma once

#include <bit/array.h>

#define BIT_TXT(str) (str)

namespace bit
{
	

	typedef char CharType_t;
	typedef bit::DefaultContainerSizeType_t StringSizeType_t;

	BITLIB_API_TEMPLATE_STRUCT bit::TDefaultBlockAllocator<StringSizeType_t>;
	BITLIB_API_TEMPLATE_STRUCT bit::TInlineBlockAllocator<CharType_t, 32, bit::TDefaultBlockAllocator<StringSizeType_t>>;
	BITLIB_API_TEMPLATE_STRUCT bit::TArray<CharType_t, StringSizeType_t, bit::TInlineBlockAllocator<CharType_t, 32, bit::TDefaultBlockAllocator<StringSizeType_t>>>;

	typedef bit::TArray<
		CharType_t,
		StringSizeType_t,
		bit::TInlineBlockAllocator<CharType_t, 32, bit::TDefaultBlockAllocator<StringSizeType_t>>
	> StringStorageType_t;

	/* ASCII String. Maybe at some point use unicode (utf-8 encoding) */
	struct BITLIB_API CString
	{
		CString();
		CString(const CharType_t* RawStr);
		CString(const CharType_t* RawStr, StringSizeType_t Len);
		CString(const CString& Other);
		CString(CString&& Other) noexcept;
		CString& operator=(const CharType_t* RawStr);
		CString& operator=(const CString& Other);
		CString& operator=(CString&& Other) noexcept;
		CString& operator+=(const CharType_t* RawStr);
		CString& operator+=(const CString& Other);
		const CharType_t* operator*() const;
		StringSizeType_t GetLength() const;
		void Copy(const CharType_t* RawStr, StringSizeType_t Len);
		void Copy(const CString& Other);
		void Append(const CharType_t* RawStr, StringSizeType_t Len);
		void Append(const CString& Other);

		friend CString operator+(const CString& LHS, const CString& RHS) { return (bit::CString(LHS) += RHS); }
		friend CString operator+(const CString& LHS, const CharType_t* RHS) { return (bit::CString(LHS) += RHS); }
		friend CString operator+(const CharType_t* LHS, const CString& RHS) { return (bit::CString(LHS) += RHS); }
		
	private:
		StringStorageType_t Storage;
	};
}
