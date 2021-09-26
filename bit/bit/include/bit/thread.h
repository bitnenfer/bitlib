#pragma once

#include <bit/types.h>
#include <bit/utils.h>

namespace bit
{
	struct BITLIB_API Thread : public NonCopyable
	{
		typedef int32_t(*ThreadFunc_t)(void*);

		Thread(Thread&& MoveRef);
		Thread();
		~Thread();
		void Start(ThreadFunc_t pFunc, size_t StackSize, void* UserData);
		void Join();
		int32_t GetId();
		bool IsValid();
		Handle_t GetHandle();
		Thread& operator=(Thread&& MoveRef);

		static int32_t GetCurrentThreadId();
		static void YieldThread();
		static void SleepThread(uint32_t Milliseconds);

	private:
		Thread(const Thread& CopyRef) = delete;
		Thread& operator=(const Thread& CopyRef) = delete;
		Handle_t Handle;
		void* UserPayload;
	};
}
