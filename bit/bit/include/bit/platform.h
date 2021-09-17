#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <bit/platform_windows.h>
#else
#include <bit/platform_null.h>
#endif

struct _BitMemNewDummy_ {};
inline void* operator new(size_t, _BitMemNewDummy_, void* Ptr) { return Ptr; }
inline void operator delete(void*, _BitMemNewDummy_, void*) {}
#define BitPlacementNew(Ptr) new(_BitMemNewDummy_(), Ptr)

namespace bit
{
	static constexpr size_t DEFAULT_ALIGNMENT = 4;
};