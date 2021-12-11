#pragma once

#include <bit/core/os/mutex.h>
#include <bit/core/types.h>
#include <bit/container/array.h>

namespace bit
{
	typedef uint32_t TlsHandle;

	BITLIB_API TlsHandle TlsAllocSlot();
	BITLIB_API void TlsFreeSlot(TlsHandle Handle);
	BITLIB_API void TlsSetValue(TlsHandle Handle, void* Value);
	BITLIB_API void* TlsGetValue(TlsHandle Handle);
}
