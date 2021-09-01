#pragma once

#include <bit/allocator.h>

namespace bit
{
	struct BITLIB_API CLinearAllocator : public IAllocator
	{
		static size_t GetRequiredAlignment();

		CLinearAllocator(const char* Name = "CLinearAllocator");
		~CLinearAllocator();

		void Initialize(void* Buffer, size_t BufferSize);
		void Terminate();
		void Reset();
		void* GetBuffer(size_t* OutSize);
		size_t CalcSize(size_t Size);

		void* Allocate(size_t Size, size_t Alignment) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		CMemoryInfo GetMemoryInfo() override;

	private:
		uint8_t* Buffer;
		size_t BufferOffset;
		size_t BufferSize;
	};
}