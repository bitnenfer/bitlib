#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <bit/core/platform_windows.h>
#else
#include <bit/core/platform_null.h>
#endif

#define BIT_PLATFORM_DEFAULT_ALLOCATOR 0

struct _BitMemNewDummy_ {};
inline void* operator new(size_t, _BitMemNewDummy_, void* Ptr) { return Ptr; }
inline void operator delete(void*, _BitMemNewDummy_, void*) {}
#define BitPlacementNew(Ptr) new(_BitMemNewDummy_(), Ptr)

#define KiB * (1024ULL)
#define MiB * (1024ULL KiB)
#define GiB * (1024ULL MiB)
#define TiB * (1024ULL GiB)
#define PiB * (1024ULL TiB)

namespace bit
{
	static constexpr size_t DEFAULT_ALIGNMENT = 4;
};