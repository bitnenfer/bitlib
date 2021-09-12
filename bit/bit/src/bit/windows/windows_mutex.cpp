#include <bit/mutex.h>
#include "windows_common.h"

bit::CMutex::CMutex()
{
	Handle = (Handle_t)CreateMutex(nullptr, false, nullptr);
}

bit::CMutex::~CMutex()
{
	CloseHandle((HANDLE)Handle);
}

void bit::CMutex::Lock()
{
	WaitForSingleObject((HANDLE)Handle, INFINITE);
}

void bit::CMutex::Unlock()
{
	ReleaseMutex((HANDLE)Handle);
}
