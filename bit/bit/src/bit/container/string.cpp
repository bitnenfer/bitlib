#include <bit/container/string.h>
#include <stdio.h>
#include <stdarg.h>

bit::String::String()
{
	Storage.AddEmpty();
}

bit::String::String(const CharType_t* RawStr) :
	String(RawStr, (SizeType_t)bit::Strlen(RawStr))
{}

bit::String::String(const CharType_t * RawStr, SizeType_t Len)
{
	Copy(RawStr, Len);
}

bit::String::String(const String& Other)
{
	Storage.Add(Other.Storage);
}

bit::String::String(String&& Other) noexcept
{
	Storage = bit::Move(Other.Storage);
}

bit::SizeType_t bit::String::GetLength() const
{
	SizeType_t Count = Storage.GetCount();
	if (Count > 0) return Count - 1;
	return 0;
}

void bit::String::Copy(const CharType_t* RawStr, SizeType_t Len)
{
	Storage.Clear();
	Storage.Add(RawStr, Len);
	Storage.AddEmpty();
}

void bit::String::Copy(const String& Other)
{
	Storage.Clear();
	Storage.Add(Other.Storage);
}

void bit::String::Append(const CharType_t* RawStr, SizeType_t Len)
{
	Storage.PopLast();
	Storage.Add(RawStr, Len);
	Storage.AddEmpty();
}

void bit::String::Append(const String& Other)
{
	Storage.PopLast();
	Storage.Add(Other.Storage);
	Storage.AddEmpty();
}

bit::StringStorage_t& bit::String::GetStorage()
{
	return Storage;
}

/*static*/ bit::String bit::String::Format(const CharType_t* Fmt, ...)
{
	char Buffer[1024];
	String Output;
	va_list VaList;
	va_start(VaList, Fmt);
	int32_t WriteSize = vsprintf_s(Buffer, 1024, Fmt, VaList);
	va_end(VaList);
	return String(Buffer, WriteSize);
}

const bit::CharType_t* bit::String::operator*() const
{
	return Storage.GetData();
}

bit::String& bit::String::operator+=(const bit::String& Other)
{
	Append(Other);
	return *this;
}

bit::String& bit::String::operator+=(const CharType_t* RawStr)
{
	Append(RawStr, (SizeType_t)bit::Strlen(RawStr));
	return *this;
}

bit::String& bit::String::operator=(String&& Other) noexcept
{
	Storage = bit::Move(Other.Storage);
	return *this;
}

bit::String& bit::String::operator=(const CharType_t* RawStr)
{
	Copy(RawStr, (SizeType_t)bit::Strlen(RawStr));
	return *this;
}

bit::String& bit::String::operator=(const String& Other)
{
	Copy(Other);
	return *this;
}

bool bit::operator==(const String& LHS, const String& RHS)
{
	return LHS.GetLength() == RHS.GetLength() && bit::Memcmp(*LHS, *RHS, LHS.GetLength());
}

bool bit::operator!=(const String& LHS, const String& RHS)
{
	return !(LHS == RHS);
}

BITLIB_API bit::String bit::FormatSize(size_t Size)
{
	if (Size >= 1 KiB && Size < 1 MiB) return String::Format("%.3lf KiB", FromKiB(Size));
	else if (Size >= 1 MiB && Size < 1 GiB) return String::Format("%.3lf MiB", FromMiB(Size));
	else if (Size >= 1 GiB && Size < 1 * 1024 GiB) return String::Format("%.3lf GiB", FromGiB(Size));
	else if (Size >= 1 TiB && Size < 1 * 1024 PiB) return String::Format("%.3lf TiB", FromGiB(Size));
	else return String::Format("%.3lf B", (double)Size);
}
