#include <bit/core/os/thread.h>
#include <bit/core/memory.h>
#include <intrin.h>
#include "../../windows_common.h"

struct ThreadPayload
{
	bit::Thread::ThreadFunc_t Func;
	void* UserData;
};

bit::Thread::Thread(Thread&& MoveRef) :
	Handle(MoveRef.Handle),
	UserPayload(MoveRef.UserPayload)
{
	MoveRef.Handle = nullptr;
	MoveRef.UserPayload = nullptr;
}

bit::Thread::Thread() :
	Handle(nullptr),
	UserPayload(nullptr)
{
}

bit::Thread::~Thread()
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

void bit::Thread::Start(ThreadFunc_t Func, size_t StackSize, void* UserData)
{
	ThreadPayload* Payload = bit::Malloc<ThreadPayload>();
	Payload->Func = Func;
	Payload->UserData = UserData;
	UserPayload = Payload;
	Handle = (Handle_t)CreateThread(nullptr, StackSize, [](LPVOID Data) -> DWORD
	{
		ThreadPayload* Payload = (ThreadPayload*)Data;
		if (Payload != nullptr && Payload->Func != nullptr)
		{
			return (DWORD)Payload->Func(Payload->UserData);
		}
		return 0;
	}, Payload, 0, nullptr);
}

void bit::Thread::Join()
{
	WaitForSingleObject((HANDLE)Handle, INFINITE);
}

int32_t bit::Thread::GetId()
{
	return GetThreadId((HANDLE)Handle);
}

bool bit::Thread::IsValid()
{
	return Handle != nullptr;
}

bit::Handle_t bit::Thread::GetHandle()
{
	return Handle;
}

bit::Thread& bit::Thread::operator=(Thread&& MoveRef)
{
	Handle = MoveRef.Handle;
	UserPayload = MoveRef.UserPayload;
	MoveRef.Handle = nullptr;
	MoveRef.UserPayload = nullptr;
	return *this;
}

/*static*/ int32_t bit::Thread::GetCurrentThreadId()
{
	return ::GetCurrentThreadId();
}

/*static*/ void bit::Thread::YieldThread()
{
	_mm_pause();
}

void bit::Thread::SleepThread(uint32_t Milliseconds)
{
	Sleep((DWORD)Milliseconds);
}
