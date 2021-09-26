#pragma once

#include <bit/types.h>
#include <bit/array.h>
#include <bit/mutex.h>

namespace bit
{

	struct BITLIB_API ThreadLocalStorage
	{
		ThreadLocalStorage();
		void SetData(void* Data);
		void* GetData();
		bool IsValid() const;
		friend bool operator==(const ThreadLocalStorage& LHS, const ThreadLocalStorage& RHS);

	private:
		ThreadLocalStorage(int32_t InTLSIndex, void* InitialData);

		friend struct ThreadLocalStorageAllocator;
		int32_t Index;
	};

	struct BITLIB_API ThreadLocalStorageHandle
	{
		ThreadLocalStorageHandle();
		ThreadLocalStorage* operator->();
		operator bool();

	private:
		friend struct ThreadLocalStorageAllocator;
		ThreadLocalStorageHandle(int32_t InIndex);
		int32_t InternalIndex;
	};

	BITLIB_API_TEMPLATE_STRUCT bit::Array<ThreadLocalStorage>;

	struct BITLIB_API ThreadLocalStorageAllocator
	{
		static ThreadLocalStorageAllocator& Get();

		ThreadLocalStorageHandle Allocate(void* InitialData = nullptr);
		void Free(ThreadLocalStorageHandle Handle);
		bit::Array<ThreadLocalStorage>& GetAllTLSAllocs();

	private:
		bit::Array<ThreadLocalStorage> AllStorage;
		bit::Mutex StorageMutex;
	};
}
