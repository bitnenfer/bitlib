#include <bit/memory.h>
#include <bit/allocator.h>
#include <bit/tlsf_allocator.h>
#include <intrin.h>
#include <stdarg.h>
#include <stdio.h>
#include "windows_common.h"

#define BIT_USE_WIN32_HEAP_AS_DEFAULT_ALLOCATOR 0

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

namespace bit
{
#if BIT_USE_WIN32_HEAP_AS_DEFAULT_ALLOCATOR
	struct CWindowsHeapAllocator : public bit::IAllocator
	{
		CWindowsHeapAllocator() :
			IAllocator::IAllocator("CWindowsHeapAllocator"),
			Heap(nullptr)
		{
			Heap = GetProcessHeap();
		}

		void* Allocate(size_t Size, size_t Alignment) override
		{
			return HeapAlloc(Heap, 0, Size);
		}
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override
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

		CMemoryUsageInfo GetMemoryUsageInfo() override
		{
			HEAP_SUMMARY Summary = {};
			CMemoryUsageInfo Info = {};
			Summary.cb = sizeof(HEAP_SUMMARY);
			if (HeapSummary(Heap, 0, &Summary))
			{
				Info.AllocatedBytes = Summary.cbAllocated;
				Info.CommittedBytes = Summary.cbCommitted;
				Info.ReservedBytes = Summary.cbReserved;
			}
			return Info;
		}
		
		HANDLE Heap;
	};
	static uint8_t HeapInitialBuffer[sizeof(CWindowsHeapAllocator)];
	IAllocator* CreateDefaultAllocator() { return BitPlacementNew(HeapInitialBuffer) CWindowsHeapAllocator(); }
#else
	static uint8_t HeapInitialBuffer[sizeof(CTLSFAllocator)];
	IAllocator* CreateDefaultAllocator() { return BitPlacementNew(HeapInitialBuffer) CTLSFAllocator("MainAllocator"); }
#endif
	static IAllocator* DefaultAllocator = nullptr;
}

bit::IAllocator& bit::GetDefaultAllocator()
{
	if (DefaultAllocator == nullptr)
	{
		DefaultAllocator = CreateDefaultAllocator();
	}
	return *DefaultAllocator;
}

void* bit::AllocateFromOS(size_t Size)
{
	return VirtualAlloc(nullptr, Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

bool bit::FreeFromOS(void* Ptr)
{
	return VirtualFree(Ptr, 0, MEM_RELEASE) == TRUE;
}

void* bit::Memcpy(void* Dst, const void* Src, size_t Num)
{
	return BitMemcpy(Dst, Src, Num);
}

void* bit::Memset(void* Ptr, int32_t Value, size_t Num)
{
	return BitMemset(Ptr, Value, Num);
}

bool bit::Memcmp(const void* A, const void* B, size_t Num)
{
	return memcmp(A, B, Num) == 0;
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
