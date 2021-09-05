#pragma once

#include <bit/allocator.h>
#include <bit/intrusive_linked_list.h>

namespace bit
{
	struct CFreeList
	{
		CFreeList* Next;
	};

	struct CHeapAllocator : public IAllocator
	{
		CHeapAllocator(const char* Name);
		~CHeapAllocator();

		virtual void* Allocate(size_t Size, size_t Alignment) override;
		virtual void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		virtual void Free(void* Pointer) override;
		virtual size_t GetSize(void* Pointer) override;
		virtual CMemoryUsageInfo GetMemoryUsageInfo() override;

	private:

	};
}
