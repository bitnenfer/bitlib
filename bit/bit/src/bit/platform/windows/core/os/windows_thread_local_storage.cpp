#include <bit/core/os/debug.h>
#include <bit/core/os/thread_local_storage.h>
#include "../../windows_common.h"

bit::TlsHandle bit::TlsAllocSlot()
{
	return ::TlsAlloc();
}

void bit::TlsFreeSlot(TlsHandle Handle)
{
	::TlsFree((DWORD)Handle);
}

void bit::TlsSetValue(TlsHandle Handle, void* Value)
{
	::TlsSetValue((DWORD)Handle, Value);
}

void* bit::TlsGetValue(TlsHandle Handle)
{
	return ::TlsGetValue(Handle);
}
