#include <bit/core/os/virtual_memory.h>
#include <bit/core/memory/allocator.h>
#include <bit/core/os/mutex.h>
#include <bit/core/memory/page_allocator.h>

namespace bit
{
	/* Based on Two-Level Segregated Fit Allocator */
	/* Source: http://www.gii.upv.es/tlsf/files/ecrts04_tlsf.pdf */
	struct BITLIB_API MediumSizeAllocator
	{
		static constexpr uint64_t MIN_ALLOCATION_SIZE = 512;
		static constexpr uint64_t MAX_ALLOCATION_SIZE = 10 MiB;
		static constexpr uint64_t ADDRESS_SPACE_SIZE = 8 GiB;
		static constexpr uint64_t SLI = 5; // How many bits we assign for second level index

	private:
		struct BITLIB_API BlockHeader
		{
			BIT_FORCEINLINE void Reset(uint64_t NewSize) { Size = 0; PrevPhysicalBlock = nullptr; SetSize(NewSize); }
			BIT_FORCEINLINE bool IsFree() const { return (Size & 0b01) > 0; }
			BIT_FORCEINLINE bool IsLastPhysicalBlock() const { return (Size & 0b10) > 0; }
			BIT_FORCEINLINE uint64_t GetSize() const { return Size & (~0b11); }
			BIT_FORCEINLINE uint64_t SetSize(uint64_t NewSize) { return (Size = (NewSize & ~0b11) | Size & 0b11); }
			BIT_FORCEINLINE void SetUsed() { Size &= ~0b1; }
			BIT_FORCEINLINE void SetFree() { Size |= 0b1; }
			BIT_FORCEINLINE void SetLastPhysicalBlock() { Size |= 0b10; }
			BIT_FORCEINLINE void RemoveLastPhysicalBlock() { Size &= ~0b10; }
			BIT_FORCEINLINE uint64_t GetFullSize() const { return GetSize() + sizeof(BlockHeader); }

			uint64_t Size;
			BlockHeader* PrevPhysicalBlock;
		};

		struct BITLIB_API BlockFreeHeader : public BlockHeader
		{
			BlockFreeHeader* NextFree;
			BlockFreeHeader* PrevFree;
		};

		struct BITLIB_API VirtualPage
		{
			BlockHeader BlockHead;
			size_t PageSize;
			VirtualPage* Prev;
			VirtualPage* Next;
		};

		struct BITLIB_API BlockMap
		{
			uint64_t FL;
			uint64_t SL;
		};

	public:
		static constexpr uint64_t COUNT_OFFSET = bit::ConstBitScanReverse(MIN_ALLOCATION_SIZE) + 1;
		static constexpr uint64_t FL_COUNT = bit::ConstBitScanReverse(MAX_ALLOCATION_SIZE) - bit::ConstBitScanReverse(MIN_ALLOCATION_SIZE) + 1;
		static constexpr uint64_t SL_COUNT = 1 << SLI;

		MediumSizeAllocator();
		~MediumSizeAllocator();

		void* Allocate(size_t Size, size_t Alignment);
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment);
		void Free(void* Pointer);
		size_t GetSize(void* Pointer);
		AllocatorMemoryInfo GetMemoryUsageInfo();
		bool CanAllocate(size_t Size, size_t Alignment);
		bool OwnsAllocation(const void* Ptr);
		size_t Compact();
	
	private:
		void AllocateVirtualMemory(size_t Size);
		bool FreeVirtualMemory(BlockFreeHeader* FreeBlock);
		size_t RoundToSlotSize(size_t Size);
		void* AllocateAligned(uint64_t Size, uint64_t Alignment);
		BlockMap MappingNoOffset(size_t Size) const;
		BlockMap Mapping(size_t Size) const;
		BlockFreeHeader* GetBlockHeaderFromPointer(void* Block) const;
		void* GetPointerFromBlockHeader(BlockHeader* Block) const;
		BlockFreeHeader* Merge(BlockFreeHeader* Block);
		BlockFreeHeader* MergeRecursive(BlockFreeHeader* Block);
		BlockFreeHeader* FindSuitableBlock(size_t Size, BlockMap& Map);
		void RemoveBlock(BlockFreeHeader* Block, BlockMap Map);
		BlockFreeHeader* Split(BlockFreeHeader* Block, uint64_t Size);
		BlockFreeHeader* SplitAligned(BlockFreeHeader* Block, uint64_t Size, uint64_t Alignment);
		void InsertBlock(BlockFreeHeader* Block, BlockMap Map);
		BlockFreeHeader* MergeBlocks(BlockFreeHeader* Left, BlockFreeHeader* Right);
		BlockFreeHeader* GetNextBlock(BlockFreeHeader* Block) const;
		size_t AdjustSize(size_t Size);
		void FillBlock(BlockHeader* Block, uint8_t Value) const;

		uint64_t FLBitmap;
		uint64_t SLBitmap[FL_COUNT];
		BlockFreeHeader* FreeBlocks[FL_COUNT][SL_COUNT];
		VirtualPage* PagesInUse;
		VirtualAddressSpace Memory;
		void* VirtualMemoryBaseAddress;
		size_t PagesInUseCount;
		size_t VirtualMemoryBaseOffset;
		size_t UsedSpaceInBytes;
		size_t AvailableSpaceInBytes;
	};
}
