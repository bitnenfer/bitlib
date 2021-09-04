#include <bit/thread.h>
#include <bit/memory.h>
#include <intrin.h>
#include "windows_common.h"

struct CThreadPayload
{
	bit::CThread::ThreadFunc_t Func;
	void* UserData;
};

static DWORD BitThreadProcess(LPVOID Data)
{
	CThreadPayload* Payload = (CThreadPayload*)Data;
	if (Payload != nullptr && Payload->Func != nullptr)
	{
		return (DWORD)Payload->Func(Payload->UserData);
	}
	return 0;
}

bit::CThread::CThread(CThread&& MoveRef) :
	Handle(MoveRef.Handle),
	UserPayload(MoveRef.UserPayload)
{
	MoveRef.Handle = nullptr;
	MoveRef.UserPayload = nullptr;
}

bit::CThread::CThread() :
	Handle(nullptr),
	UserPayload(nullptr)
{
}

bit::CThread::~CThread()
{
	if (Handle != nullptr)
	{
		Join();
		CloseHandle(Handle);
		Handle = nullptr;
	}
	if (UserPayload != nullptr)
	{
		bit::Free(UserPayload);
	}
}

void bit::CThread::Start(ThreadFunc_t Func, size_t StackSize, void* UserData)
{
	CThreadPayload* Payload = bit::Malloc<CThreadPayload>();
	Payload->Func = Func;
	Payload->UserData = UserData;
	UserPayload = Payload;
	Handle = (Handle_t)CreateThread(nullptr, StackSize, &BitThreadProcess, Payload, 0, nullptr);
}

void bit::CThread::Join()
{
	WaitForSingleObject((HANDLE)Handle, INFINITE);
}

int32_t bit::CThread::GetId()
{
	return GetThreadId((HANDLE)Handle);
}

bool bit::CThread::IsValid()
{
	return Handle != nullptr;
}

bit::Handle_t bit::CThread::GetHandle()
{
	return Handle;
}

bit::CThread& bit::CThread::operator=(CThread&& MoveRef)
{
	Handle = MoveRef.Handle;
	UserPayload = MoveRef.UserPayload;
	MoveRef.Handle = nullptr;
	MoveRef.UserPayload = nullptr;
	return *this;
}

/*static*/ int32_t bit::CThread::GetCurrentThreadId()
{
	return ::GetCurrentThreadId();
}

/*static*/ uint64_t bit::CThread::GetLogicalProcessorCount()
{
	SYSTEM_INFO Info = {};
	GetSystemInfo(&Info);
	return Info.dwNumberOfProcessors;
}

/*static*/ void bit::CThread::YieldThread()
{
	_mm_pause();
}

void bit::CThread::SleepThread(uint32_t Milliseconds)
{
	Sleep((DWORD)Milliseconds);
}
