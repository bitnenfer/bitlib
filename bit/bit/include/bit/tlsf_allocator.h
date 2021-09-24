#include <bit/allocator.h>
#include <bit/memory.h>
#include <bit/virtual_memory.h>
#include <bit/mutex.h>

namespace bit
{
	/* Two-Level Segregated Fit Allocator */
	/* Source: http://www.gii.upv.es/tlsf/files/ecrts04_tlsf.pdf */
	struct BITLIB_API CTLSFAllocator : public bit::IAllocator
	{
	private:
		struct BITLIB_API CBlockHeader
		{
			BIT_FORCEINLINE void Reset(uint32_t NewSize) { Size = 0; PrevPhysicalBlock = nullptr; SetSize(NewSize); }
			BIT_FORCEINLINE bool IsFree() const { return (Size & 0b01) > 0; }
			BIT_FORCEINLINE bool IsLastPhysicalBlock() const { return (Size & 0b10) > 0; }
			BIT_FORCEINLINE uint32_t GetSize() const { return Size & (~0b11); }
			BIT_FORCEINLINE uint32_t SetSize(uint32_t NewSize) { return (Size = (NewSize & ~0b11) | Size & 0b11); }
			BIT_FORCEINLINE void SetUsed() { Size &= ~0b1; }
			BIT_FORCEINLINE void SetFree() { Size |= 0b1; }
			BIT_FORCEINLINE void SetLastPhysicalBlock() { Size |= 0b10; }
			BIT_FORCEINLINE void RemoveLastPhysicalBlock() { Size &= ~0b10; }
			BIT_FORCEINLINE uint32_t GetFullSize() const { return GetSize() + sizeof(CBlockHeader); }

			uint32_t Size;
			CBlockHeader* PrevPhysicalBlock;
		};

		struct BITLIB_API CMemoryPool
		{
			bit::CVirtualAddressSpace VirtualMemory;
			CMemoryPool* Next;
			uint32_t UsableSize;
			void* BaseAddress;
		};

		struct BITLIB_API CBlockFree
		{
			CBlockHeader Header;
			CBlockFree* NextFree;
			CBlockFree* PrevFree;
		};

		struct BITLIB_API CBlockMap
		{
			uint64_t FL;
			uint64_t SL;
		};

	public:
		static constexpr uint64_t SLI = 4;
		static constexpr uint64_t MAX_POOL_SIZE = 4 GiB;
		static constexpr uint64_t FL_COUNT = bit::ConstBitScanReverse(MAX_POOL_SIZE);
		static constexpr uint64_t SL_COUNT = 1 << SLI;
		static constexpr uint64_t MIN_BLOCK_SIZE = sizeof(CBlockHeader);
		static constexpr uint64_t NUM_LIST = (1 << SLI) * (FL_COUNT - bit::ConstBitScanReverse64(MIN_BLOCK_SIZE));

		CTLSFAllocator(size_t InitialPoolSize, const char* Name = "TLSF Allocator");
		CTLSFAllocator(const char* Name = "TLSF Allocator");
		~CTLSFAllocator();

		void* Allocate(size_t Size, size_t Alignment) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		CMemoryUsageInfo GetMemoryUsageInfo() override;
		size_t TrimMemory();
	
	private:
		bool IsPoolReleasable(CMemoryPool* Pool);
		void CreateAndInsertMemoryPool(size_t PoolSize);
		void InsertMemoryPool(CMemoryPool* NewMemoryPool);
		CMemoryPool* CreateMemoryPool(size_t PoolSize);
		CBlockMap Mapping(size_t Size) const;
		CBlockFree* GetBlockHeaderFromPointer(void* Block) const;
		void* GetPointerFromBlockHeader(CBlockFree* Block) const;
		CBlockFree* Merge(CBlockFree* Block);
		CBlockFree* FindSuitableBlock(size_t Size, CBlockMap& Map);
		void RemoveBlock(CBlockFree* Block, CBlockMap Map);
		CBlockFree* Split(CBlockFree* Block, uint32_t Size);
		void InsertBlock(CBlockFree* Block, CBlockMap Map);
		CBlockFree* MergeBlocks(CBlockFree* Left, CBlockFree* Right);
		CBlockFree* GetNextBlock(CBlockFree* Block) const;
		size_t AdjustSize(size_t Size);

		uint32_t FLBitmap;
		uint32_t SLBitmap[FL_COUNT];
		CBlockFree* FreeBlocks[FL_COUNT][SL_COUNT];
		CMemoryPool* MemoryPoolList;
		size_t AllocatedAmount;
		CMutex Mutex;
	};
}
