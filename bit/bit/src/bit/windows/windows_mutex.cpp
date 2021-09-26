#include <bit/mutex.h>
#include "windows_common.h"

bit::Mutex::Mutex()
{
	Handle = (Handle_t)CreateMutex(nullptr, false, nullptr);
}

bit::Mutex::~Mutex()
{
	CloseHandle((HANDLE)Handle);
}

void bit::Mutex::Lock()
{
	WaitForSingleObject((HANDLE)Handle, INFINITE);
}

void bit::Mutex::Unlock()
{
	ReleaseMutex((HANDLE)Handle);
}
