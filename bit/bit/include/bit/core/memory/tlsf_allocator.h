#include <bit/core/os/virtual_memory.h>
#include <bit/core/memory/allocator.h>
#include <bit/core/memory.h>
#include <bit/core/os/mutex.h>
#include <bit/container/intrusive_linked_list.h>

namespace bit
{
	using TLSFSizeType_t = uint32_t;

	/* Two-Level Segregated Fit Allocator */
	/* Source: http://www.gii.upv.es/tlsf/files/ecrts04_tlsf.pdf */
	struct BITLIB_API TLSFAllocator : public bit::IAllocator
	{
	private:
		struct BITLIB_API BlockHeader
		{
			BIT_FORCEINLINE void Reset(TLSFSizeType_t NewSize) { Size = 0; PrevPhysicalBlock = nullptr; SetSize(NewSize); }
			BIT_FORCEINLINE bool IsFree() const { return (Size & 0b01) > 0; }
			BIT_FORCEINLINE bool IsLastPhysicalBlock() const { return (Size & 0b10) > 0; }
			BIT_FORCEINLINE TLSFSizeType_t GetSize() const { return Size & (~0b11); }
			BIT_FORCEINLINE TLSFSizeType_t SetSize(TLSFSizeType_t NewSize) { return (Size = (NewSize & ~0b11) | Size & 0b11); }
			BIT_FORCEINLINE void SetUsed() { Size &= ~0b1; }
			BIT_FORCEINLINE void SetFree() { Size |= 0b1; }
			BIT_FORCEINLINE void SetLastPhysicalBlock() { Size |= 0b10; }
			BIT_FORCEINLINE void RemoveLastPhysicalBlock() { Size &= ~0b10; }
			BIT_FORCEINLINE TLSFSizeType_t GetFullSize() const { return GetSize() + sizeof(BlockHeader); }

			TLSFSizeType_t Size;
			BlockHeader* PrevPhysicalBlock;
		};

		struct BITLIB_API BlockFreeHeader : public BlockHeader
		{
			BlockFreeHeader* NextFree;
			BlockFreeHeader* PrevFree;
		};

		struct BITLIB_API MemoryPool
		{
			VirtualAddressSpace Memory;
			MemoryPool* Next;
			TLSFSizeType_t PoolSize;
			void* BaseAddress;
		};

		struct BITLIB_API BlockMap
		{
			TLSFSizeType_t FL;
			TLSFSizeType_t SL;
		};

	public:
		static constexpr TLSFSizeType_t SLI = 4; // How many bits we assign for second level index
		static constexpr TLSFSizeType_t MAX_POOL_SIZE = (TLSFSizeType_t)ConstNextPow2(512 MiB);
		static constexpr TLSFSizeType_t MIN_ALLOC_SIZE = 1 << SLI;
		static constexpr TLSFSizeType_t COUNT_OFFSET = bit::ConstBitScanReverse(MIN_ALLOC_SIZE) + 1;
		static constexpr TLSFSizeType_t FL_COUNT = bit::ConstBitScanReverse(MAX_POOL_SIZE) - COUNT_OFFSET + 1;
		static constexpr TLSFSizeType_t SL_COUNT = 1 << SLI;

		TLSFAllocator(size_t InitialPoolSize, const char* Name = "TLSF Allocator");
		TLSFAllocator(const char* Name = "TLSF Allocator");
		~TLSFAllocator();

		void* Allocate(size_t Size, size_t Alignment) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		MemoryUsageInfo GetMemoryUsageInfo() override;
		size_t TrimMemory();
	
	private:
		void* AllocateAligned(TLSFSizeType_t Size, TLSFSizeType_t Alignment);
		bool IsPoolReleasable(MemoryPool* Pool);
		void AddNewPool(size_t PoolSize);
		BlockMap Mapping(size_t Size) const;
		BlockFreeHeader* GetBlockHeaderFromPointer(void* Block) const;
		void* GetPointerFromBlockHeader(BlockHeader* Block) const;
		BlockFreeHeader* Merge(BlockFreeHeader* Block);
		BlockFreeHeader* MergeRecursive(BlockFreeHeader* Block);
		BlockFreeHeader* FindSuitableBlock(size_t Size, BlockMap& Map);
		void RemoveBlock(BlockFreeHeader* Block, BlockMap Map);
		BlockFreeHeader* Split(BlockFreeHeader* Block, TLSFSizeType_t Size);
		BlockFreeHeader* SplitAligned(BlockFreeHeader* Block, TLSFSizeType_t Size, TLSFSizeType_t Alignment);
		void InsertBlock(BlockFreeHeader* Block, BlockMap Map);
		BlockFreeHeader* MergeBlocks(BlockFreeHeader* Left, BlockFreeHeader* Right);
		BlockFreeHeader* GetNextBlock(BlockFreeHeader* Block) const;
		size_t AdjustSize(size_t Size);
		void FillBlock(BlockHeader* Block, uint8_t Value) const;

		TLSFSizeType_t FLBitmap;
		TLSFSizeType_t SLBitmap[FL_COUNT];
		BlockFreeHeader* FreeBlocks[FL_COUNT][SL_COUNT];
		MemoryPool* MemoryPoolList;
		size_t MemoryPoolCount;
		size_t UsedSpaceInBytes;
		size_t AvailableSpaceInBytes;
		Mutex AccessLock;
	};
}
