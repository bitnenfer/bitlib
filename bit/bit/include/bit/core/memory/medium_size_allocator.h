#include <bit/core/os/virtual_memory.h>
#include <bit/core/memory/allocator.h>
#include <bit/core/os/mutex.h>
#include <bit/core/memory/page_allocator.h>

#define BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY 1

namespace bit
{
	/* Based on Two-Level Segregated Fit Allocator */
	/* Source: http://www.gii.upv.es/tlsf/files/ecrts04_tlsf.pdf */
	struct BITLIB_API MediumSizeAllocator : public bit::IAllocator
	{
		static constexpr bool USING_VIRTUAL_MEMORY = BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY;

		using SizeType_t = uint64_t;
	private:
		struct BITLIB_API BlockHeader
		{
			BIT_FORCEINLINE void Reset(SizeType_t NewSize) { Size = 0; PrevPhysicalBlock = nullptr; SetSize(NewSize); }
			BIT_FORCEINLINE bool IsFree() const { return (Size & 0b01) > 0; }
			BIT_FORCEINLINE bool IsLastPhysicalBlock() const { return (Size & 0b10) > 0; }
			BIT_FORCEINLINE SizeType_t GetSize() const { return Size & (~0b11); }
			BIT_FORCEINLINE SizeType_t SetSize(SizeType_t NewSize) { return (Size = (NewSize & ~0b11) | Size & 0b11); }
			BIT_FORCEINLINE void SetUsed() { Size &= ~0b1; }
			BIT_FORCEINLINE void SetFree() { Size |= 0b1; }
			BIT_FORCEINLINE void SetLastPhysicalBlock() { Size |= 0b10; }
			BIT_FORCEINLINE void RemoveLastPhysicalBlock() { Size &= ~0b10; }
			BIT_FORCEINLINE SizeType_t GetFullSize() const { return GetSize() + sizeof(BlockHeader); }

			SizeType_t Size;
			BlockHeader* PrevPhysicalBlock;
		};

		struct BITLIB_API BlockFreeHeader : public BlockHeader
		{
			BlockFreeHeader* NextFree;
			BlockFreeHeader* PrevFree;
		};

#if !BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
		struct BITLIB_API MemoryPool
		{
			BlockHeader BlockHead; /* This must be at the top */
			VirtualAddressSpace Memory;
			MemoryPool* Prev;
			MemoryPool* Next;
			SizeType_t PoolSize;
			void* BaseAddress;
		};
#endif

		struct BITLIB_API BlockMap
		{
			SizeType_t FL;
			SizeType_t SL;
		};

	public:
		static constexpr SizeType_t SLI = 5; // How many bits we assign for second level index
		static constexpr SizeType_t MAX_ALLOCATION_SIZE = 4 MiB;
		static constexpr SizeType_t MIN_ALLOCATION_SIZE = 4 KiB;
		static constexpr SizeType_t COUNT_OFFSET = bit::ConstBitScanReverse(MIN_ALLOCATION_SIZE) + 1;
		static constexpr SizeType_t FL_COUNT = bit::ConstBitScanReverse(MAX_ALLOCATION_SIZE) - COUNT_OFFSET + 1;
		static constexpr SizeType_t SL_COUNT = 1 << SLI;
		static constexpr SizeType_t ADDRESS_SPACE_SIZE = 8 GiB;

		MediumSizeAllocator(const char* Name = "Medium Size Allocator");
		~MediumSizeAllocator();

		void* Allocate(size_t Size, size_t Alignment) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		AllocatorMemoryInfo GetMemoryUsageInfo() override;
		bool CanAllocate(size_t Size, size_t Alignment) override;
		bool OwnsAllocation(const void* Ptr) override;
		size_t Compact() override;
	
	private:
#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
		void AllocateVirtualMemory(size_t Size);
		bool FreeVirtualMemory(BlockFreeHeader* FreeBlock);
#else
		void ReleaseMemoryPool(MemoryPool* Pool);
		bool ReleaseUnusedMemoryPool(BlockFreeHeader* FreeBlock);
		bool IsPoolReleasable(MemoryPool* Pool);
		bool RemoveMemoryPoolFreeBlocks(MemoryPool* Pool);
		void AddNewPool(size_t PoolSize);
#endif

		void* AllocateAligned(SizeType_t Size, SizeType_t Alignment);
		BlockMap Mapping(size_t Size) const;
		BlockFreeHeader* GetBlockHeaderFromPointer(void* Block) const;
		void* GetPointerFromBlockHeader(BlockHeader* Block) const;
		BlockFreeHeader* Merge(BlockFreeHeader* Block);
		BlockFreeHeader* MergeRecursive(BlockFreeHeader* Block);
		BlockFreeHeader* FindSuitableBlock(size_t Size, BlockMap& Map);
		void RemoveBlock(BlockFreeHeader* Block, BlockMap Map);
		BlockFreeHeader* Split(BlockFreeHeader* Block, SizeType_t Size);
		BlockFreeHeader* SplitAligned(BlockFreeHeader* Block, SizeType_t Size, SizeType_t Alignment);
		void InsertBlock(BlockFreeHeader* Block, BlockMap Map);
		BlockFreeHeader* MergeBlocks(BlockFreeHeader* Left, BlockFreeHeader* Right);
		BlockFreeHeader* GetNextBlock(BlockFreeHeader* Block) const;
		size_t AdjustSize(size_t Size);
		void FillBlock(BlockHeader* Block, uint8_t Value) const;

		SizeType_t FLBitmap;
		SizeType_t SLBitmap[FL_COUNT];
		BlockFreeHeader* FreeBlocks[FL_COUNT][SL_COUNT];
#if BIT_MEDIUM_SIZE_ALLOCATOR_ENABLE_VIRTUAL_MEMORY
		PageAllocator VirtualMemory;
#else
		MemoryPool* MemoryPoolList;
		size_t MemoryPoolCount;
#endif
		size_t UsedSpaceInBytes;
		size_t AvailableSpaceInBytes;
	};
}
