#include <bit/string.h>
#include <stdio.h>
#include <stdarg.h>

bit::CString::CString()
{
	Storage.AddEmpty();
}

bit::CString::CString(const CharType_t* RawStr) :
	CString(RawStr, (StringSizeType_t)bit::Strlen(RawStr))
{}

bit::CString::CString(const CharType_t * RawStr, StringSizeType_t Len)
{
	Copy(RawStr, Len);
}

bit::CString::CString(const CString& Other)
{
	Storage.Add(Other.Storage);
}

bit::CString::CString(CString&& Other) noexcept
{
	Storage = bit::Move(Other.Storage);
}

bit::StringSizeType_t bit::CString::GetLength() const
{
	StringSizeType_t Count = Storage.GetCount();
	if (Count > 0) return Count - 1;
	return 0;
}

void bit::CString::Copy(const CharType_t* RawStr, StringSizeType_t Len)
{
	Storage.Reset();
	Storage.Add(RawStr, Len);
	Storage.AddEmpty();
}

void bit::CString::Copy(const CString& Other)
{
	Storage.Reset();
	Storage.Add(Other.Storage);
}

void bit::CString::Append(const CharType_t* RawStr, StringSizeType_t Len)
{
	Storage.PopLast();
	Storage.Add(RawStr, Len);
	Storage.AddEmpty();
}

void bit::CString::Append(const CString& Other)
{
	Storage.PopLast();
	Storage.Add(Other.Storage);
	Storage.AddEmpty();
}

/*static*/ bit::CString bit::CString::Format(const CharType_t* Fmt, ...)
{
	char Buffer[1024];
	CString Output;
	va_list VaList;
	va_start(VaList, Fmt);
	int32_t WriteSize = vsprintf_s(Buffer, 1024, Fmt, VaList);
	va_end(VaList);
	return CString(Buffer, WriteSize);
}

const bit::CharType_t* bit::CString::operator*() const
{
	return Storage.GetData();
}

bit::CString& bit::CString::operator+=(const bit::CString& Other)
{
	Append(Other);
	return *this;
}

bit::CString& bit::CString::operator+=(const CharType_t* RawStr)
{
	Append(RawStr, (StringSizeType_t)bit::Strlen(RawStr));
	return *this;
}

bit::CString& bit::CString::operator=(CString&& Other) noexcept
{
	Storage = bit::Move(Other.Storage);
	return *this;
}

bit::CString& bit::CString::operator=(const CharType_t* RawStr)
{
	Copy(RawStr, (StringSizeType_t)bit::Strlen(RawStr));
	return *this;
}

bit::CString& bit::CString::operator=(const CString& Other)
{
	Copy(Other);
	return *this;
}