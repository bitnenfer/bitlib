#pragma once

#include <bit/types.h>
#include <bit/array.h>
#include <bit/mutex.h>

namespace bit
{

	struct BITLIB_API CTLStorage
	{
		CTLStorage();
		void SetData(void* Data);
		void* GetData();
		bool IsValid() const;
		friend bool operator==(const CTLStorage& LHS, const CTLStorage& RHS);

	private:
		CTLStorage(int32_t InTLSIndex, void* InitialData);

		friend struct CTLSAllocator;
		int32_t Index;
	};

	struct BITLIB_API CTLSHandle
	{
		CTLSHandle();
		CTLStorage* operator->();
		operator bool();

	private:
		friend struct CTLSAllocator;
		CTLSHandle(int32_t InIndex);
		int32_t InternalIndex;
	};

	BITLIB_API_TEMPLATE_STRUCT bit::TDefaultBlockAllocator<bit::DefaultContainerSizeType_t>;
	BITLIB_API_TEMPLATE_STRUCT bit::TArray<CTLStorage>;

	struct BITLIB_API CTLSAllocator
	{
		static CTLSAllocator& Get();

		CTLSHandle Allocate(void* InitialData = nullptr);
		void Free(CTLSHandle Handle);
		bit::TArray<CTLStorage>& GetAllTLSAllocs();

	private:
		bit::TArray<CTLStorage> AllStorage;
		bit::CMutex StorageMutex;
	};
}
