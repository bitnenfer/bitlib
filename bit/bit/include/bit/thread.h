#pragma once

#include <bit/types.h>

namespace bit
{
	struct BITLIB_API CThread
	{
		typedef int32_t(*ThreadFunc_t)(void*);

		CThread(CThread&& MoveRef);
		CThread();
		~CThread();
		void Start(ThreadFunc_t pFunc, size_t StackSize, void* UserData);
		void Join();
		int32_t GetId();
		bool IsValid();
		Handle_t GetHandle();
		CThread& operator=(CThread&& MoveRef);

		static int32_t GetCurrentThreadId();
		static void YieldThread();
		static void SleepThread(uint32_t Milliseconds);

	private:
		CThread(const CThread& CopyRef) = delete;
		CThread& operator=(const CThread& CopyRef) = delete;
		Handle_t Handle;
		void* UserPayload;
	};
}
