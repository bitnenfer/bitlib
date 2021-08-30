#include <bit/memory.h>
#include <bit/allocator.h>
#include <intrin.h>
#include <stdarg.h>
#include <stdio.h>
#include "windows_common.h"

#pragma function(memcpy)
void* BitMemcpy(void* Dst, const void* Src, size_t Num)
{
	__movsb((uint8_t*)Dst, (uint8_t*)Src, Num);
	return Dst;
}

#pragma function(memset)
void* BitMemset(void* Ptr, int32_t Value, size_t Num)
{
	__stosb((uint8_t*)Ptr, (uint8_t)Value, Num);
	return Ptr;
}

extern "C" int memcmp(const void* A, const void* B, size_t Num);
extern "C" size_t strlen(const char* Str);
extern "C" const char* strstr(const char* A, const char* B);
extern "C" int strcmp(const char* A, const char* B);

struct WindowsHeapAllocator : public bit::IAllocator
{
	WindowsHeapAllocator() :
		Heap(nullptr)
	{
		Heap = GetProcessHeap();
	}

	void* Alloc(size_t Size, size_t Alignment) override
	{
		return HeapAlloc(Heap, 0, Size);
	}
	void* Realloc(void* Pointer, size_t Size, size_t Alignment) override
	{
		if (Pointer != nullptr)
			return HeapReAlloc(Heap, 0, Pointer, Size);
		else
			return HeapAlloc(Heap, 0, Size);
	}
	void Free(void* Pointer) override
	{
		HeapFree(Heap, 0, Pointer);
	}
	size_t GetSize(void* Pointer) override
	{
		return HeapSize(Heap, 0, Pointer);
	}
	size_t GetTotalUsedMemory() override
	{
		return 0;
	}
	const char* GetName() override 
	{
		return "WindowsHeapAllocator";
	}

	HANDLE Heap;
};

static uint8_t HeapInitialBuffer[sizeof(WindowsHeapAllocator)];
static WindowsHeapAllocator* DefaultAllocator = nullptr;

bit::IAllocator& bit::GetDefaultAllocator()
{
	if (DefaultAllocator == nullptr)
	{
		DefaultAllocator = BitPlacementNew(HeapInitialBuffer) WindowsHeapAllocator();
	}
	return *DefaultAllocator;
}

void* bit::Memcpy(void* Dst, const void* Src, size_t Num)
{
	return BitMemcpy(Dst, Src, Num);
}

void* bit::Memset(void* Ptr, int32_t Value, size_t Num)
{
	return BitMemset(Ptr, Value, Num);
}

int bit::Memcmp(const void* A, const void* B, size_t Num)
{
	return memcmp(A, B, Num);
}

size_t bit::Strlen(const char* Str)
{
	return strlen(Str);
}

bool bit::StrContains(const char* A, const char* B, size_t* Offset)
{
	const char* Result = strstr(A, B);
	if (Result != nullptr)
	{
		if (Offset != nullptr)
		{
			*Offset = (size_t)((uintptr_t)Result - (uintptr_t)A);
		}
		return true;
	}
	return false;
}

bool bit::Strcmp(const char* A, const char* B)
{
	return strcmp(A, B) == 0;
}

size_t bit::Fmt(char* Buffer, size_t BufferSize, const char* Fmt, ...)
{
	va_list VaArgs;
	va_start(VaArgs, Fmt);
	int32_t Written = vsprintf_s(Buffer, BufferSize, Fmt, VaArgs);
	va_end(VaArgs);
	return (size_t)Written;
}

const char* bit::TempFmtString(const char* Fmt, ...)
{
	static char Buffer[4096 * 3] = {};
	static uint32_t Index = 0;
	char* CurrBuffer = &Buffer[Index * 4096];
	va_list VaArgs;
	va_start(VaArgs, Fmt);
	vsprintf_s(CurrBuffer, 4096, Fmt, VaArgs);
	va_end(VaArgs);
	Index = (Index + 1) % 3;
	return CurrBuffer;
}
