#pragma once

#include <bit/core/memory/allocator.h>
#include <bit/core/os/virtual_memory.h>

namespace bit
{
	struct BITLIB_API SmallSizeAllocator : public bit::IAllocator
	{
		/* Since we're mostly on 64 bit system the min allocation size is size of pointer */
		static constexpr size_t MIN_ALLOCATION_SIZE = 8; 
		static constexpr size_t NUM_OF_SIZES = 16;
		static constexpr size_t MAX_ALLOCATION_SIZE = MIN_ALLOCATION_SIZE * NUM_OF_SIZES;

	private:
		struct BITLIB_API VirtualBlock
		{
			void* Start;
			void* End;
			size_t Committed;
			int64_t Allocated;

			BIT_FORCEINLINE size_t GetSize() const { return bit::PtrDiff(Start, End); }
			BIT_FORCEINLINE bool OwnsPointer(const void* Ptr) const { return bit::PtrInRange(Ptr, Start, End); }
		};

		struct BITLIB_API FreeBlock
		{
			FreeBlock* Next;
		};

	public:
		SmallSizeAllocator(size_t ReservedSpaceInBytes, const char* Name = "Small Allocator");
		SmallSizeAllocator(const char* Name = "Small Allocator");
		void* Allocate(size_t Size, size_t Alignment) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		MemoryUsageInfo GetMemoryUsageInfo() override;
		bool CanAllocate(size_t Size, size_t Alignment) override;
		bool OwnsAllocation(const void* Ptr) override;

	private:
		void* AllocateFromVirtualBlock(size_t BlockIndex, size_t Size);
		size_t SelectBlockIndex(size_t Size);
		size_t GetBlockSize(size_t BlockIndex);
		FreeBlock* PopFreeBlock(size_t BlockIndex);

		VirtualAddressSpace Memory;
		VirtualBlock Blocks[NUM_OF_SIZES];
		FreeBlock* FreeLists[NUM_OF_SIZES];
		size_t UsedSpaceInBytes;
	};
}
