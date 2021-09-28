#pragma once

#include <bit/core/memory/allocator.h>
#include <bit/core/os/virtual_memory.h>

namespace bit
{
	struct SmallAllocator : public bit::IAllocator
	{
		/* Since we're mostly on 64 bit system the min allocation size is size of pointer */
		static constexpr size_t MIN_ALLOCATION_SIZE = 8; 

		/* Must be divisible by 8 */
		static constexpr size_t MAX_ALLOCATION_SIZE = 128; 
		static_assert((MAX_ALLOCATION_SIZE% MIN_ALLOCATION_SIZE) == 0, "Must be divisible by the the minimum allocation size.");
		
		static constexpr size_t NUM_OF_SIZES = MAX_ALLOCATION_SIZE / MIN_ALLOCATION_SIZE;
		

	private:
		struct AddressRange
		{
			void* Start;
			void* End;
			BIT_FORCEINLINE size_t GetSize() const { return bit::PtrDiff(Start, End); }
			BIT_FORCEINLINE bool OwnsPointer(const void* Ptr) const { return bit::PtrInRange(Ptr, Start, End); }
		};

		struct FreeBlock
		{
			FreeBlock* Next;
		};

	public:
		SmallAllocator(size_t ReservedSpaceInBytes, const char* Name = "Small Allocator");
		SmallAllocator(const char* Name = "Small Allocator");
		void* Allocate(size_t Size, size_t Alignment) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		MemoryUsageInfo GetMemoryUsageInfo() override;

	private:
		VirtualAddressSpace Memory;
		AddressRange AddressRanges[NUM_OF_SIZES];
		FreeBlock* FreeLists[NUM_OF_SIZES];
		size_t UsedSpaceInBytes;
	};
}
