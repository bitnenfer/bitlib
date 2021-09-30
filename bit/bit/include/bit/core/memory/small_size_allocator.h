#pragma once

#include <bit/core/memory/allocator.h>
#include <bit/core/os/virtual_memory.h>

#define BIT_SMALL_ALLOCATOR_USE_4BYTE_MIN 1

namespace bit
{
	struct BITLIB_API SmallSizeAllocator : public bit::IAllocator
	{
	#if BIT_SMALL_ALLOCATOR_USE_4BYTE_MIN
		static constexpr size_t MIN_ALLOCATION_SIZE = 4; 
	#else
		static constexpr size_t MIN_ALLOCATION_SIZE = 8;
	#endif
		static constexpr size_t NUM_OF_SIZES = 1 << 10;
		static constexpr size_t MAX_ALLOCATION_SIZE = MIN_ALLOCATION_SIZE * NUM_OF_SIZES;
		static constexpr size_t ADDRESS_SPACE_SIZE = 512 MiB;
		static constexpr size_t RANGE_SIZE = ADDRESS_SPACE_SIZE / NUM_OF_SIZES;
		static_assert((ADDRESS_SPACE_SIZE % NUM_OF_SIZES) == 0, "ADDRESS_SPACE_SIZE must be divisible by the number of sizes");

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
		#if BIT_SMALL_ALLOCATOR_USE_4BYTE_MIN
			uint32_t Next;
		#else
			FreeBlock* Next;
		#endif
		};

	public:
		SmallSizeAllocator(const char* Name = "Small Size Allocator");
		void* Allocate(size_t Size, size_t Alignment) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		AllocatorMemoryInfo GetMemoryUsageInfo() override;
		bool CanAllocate(size_t Size, size_t Alignment) override;
		bool OwnsAllocation(const void* Ptr) override;

	private:
	#if BIT_SMALL_ALLOCATOR_USE_4BYTE_MIN
		FreeBlock* GetBlock(size_t BlockIndex, uint32_t Next);
		uint32_t GetOffset(size_t BlockIndex, FreeBlock* Block);
	#endif
		void* AllocateFromVirtualBlock(size_t BlockIndex, size_t Size);
		size_t SelectBlockIndex(size_t Size);
		size_t GetBlockSize(size_t BlockIndex);
		FreeBlock* PopFreeBlock(size_t BlockIndex);

		VirtualAddressSpace Memory;
		VirtualBlock Blocks[NUM_OF_SIZES];
	#if BIT_SMALL_ALLOCATOR_USE_4BYTE_MIN
		uint32_t FreeLists[NUM_OF_SIZES];
	#else
		FreeBlock* FreeLists[NUM_OF_SIZES];
	#endif
		size_t UsedSpaceInBytes;
	};
}
