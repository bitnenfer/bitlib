#pragma once

#include <bit/core/memory.h>
#include <bit/core/os/debug.h>
#include <bit/utility/utility.h>

namespace bit
{
	typedef int64_t SizeType_t;
	//static_assert(bit::IsSigned<SizeType_t>::Value, "Size type must be signed");

	struct BITLIB_API DefaultHeapAllocator
	{
		void* Allocate(void* Original, SizeType_t Size, SizeType_t Count)
		{
			return bit::Realloc(Original, (size_t)(Size * Count), DEFAULT_ALIGNMENT);
		}

		void Free(void* Ptr)
		{
			bit::Free(Ptr);
		}
	};

	struct BITLIB_API BlockStorage
	{
		BlockStorage(BlockStorage&& InBlockAllocator) noexcept :
			Block(InBlockAllocator.Block),
			AllocationSize(InBlockAllocator.AllocationSize),
			BackingAllocator(InBlockAllocator.BackingAllocator)
		{
			InBlockAllocator.Block = nullptr;
			InBlockAllocator.AllocationSize = 0;
			InBlockAllocator.BackingAllocator = nullptr;
		}
		
		BlockStorage(const BlockStorage& InAllocator) :
			Block(nullptr),
			AllocationSize(0),
			BackingAllocator(&bit::GetGlobalAllocator())
		{
			if (InAllocator.IsValid())
			{
				Allocate(InAllocator.GetBlockSize(), 1);
				bit::Memcpy(Block, InAllocator.GetBlock(), InAllocator.GetBlockSize());
			}
		}

		BlockStorage(IAllocator* Allocator, const BlockStorage& InAllocator) :
			Block(nullptr),
			AllocationSize(0),
			BackingAllocator(Allocator)
		{
			if (InAllocator.IsValid())
			{
				Allocate(InAllocator.GetBlockSize(), 1);
				bit::Memcpy(Block, InAllocator.GetBlock(), InAllocator.GetBlockSize());
			}
		}

		BlockStorage& operator=(const BlockStorage& InAllocator)
		{
			Free();
			if (InAllocator.IsValid())
			{
				Allocate(InAllocator.GetBlockSize(), 1);
				bit::Memcpy(Block, InAllocator.GetBlock(), InAllocator.GetBlockSize());
			}
			return *this;
		}

		BlockStorage& operator=(BlockStorage&& InBlockAllocator) noexcept
		{
			Block = InBlockAllocator.Block;
			AllocationSize = InBlockAllocator.AllocationSize;
			BackingAllocator = InBlockAllocator.BackingAllocator;
			InBlockAllocator.Block = nullptr;
			InBlockAllocator.AllocationSize = 0;
			InBlockAllocator.BackingAllocator = nullptr;
			return *this; 
		}

		BlockStorage() :
			Block(nullptr),
			AllocationSize(0),
			BackingAllocator(&bit::GetGlobalAllocator())
		{}

		BlockStorage(IAllocator* Allocator) :
			Block(nullptr),
			AllocationSize(0),
			BackingAllocator(Allocator)
		{}

		~BlockStorage()
		{
			Free();
		}

		void Allocate(SizeType_t Size, SizeType_t Count)
		{
			Block = BackingAllocator->Reallocate(Block, Size * Count, bit::DEFAULT_ALIGNMENT);
			AllocationSize = Size * Count;
		}

		void Free()
		{
			if (IsValid())
			{
				BackingAllocator->Free(Block);
				Block = nullptr;
				AllocationSize = 0;
			}
		}

		SizeType_t GetBlockSize() const { return AllocationSize; }
		void* GetBlock() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:
		void* Block;
		SizeType_t AllocationSize;
		IAllocator* BackingAllocator;
	};

	template<typename T, size_t Capacity>
	struct FixedBlockStorage
	{
		FixedBlockStorage() = default;

		FixedBlockStorage(IAllocator* Allocator)
		{
			BIT_ALWAYS_PANIC_MSG("FixedBlockStorage can't use an IAllocator as a backing allocator.");
		}

		void Allocate(SizeType_t Size, SizeType_t Count)
		{
			BIT_ASSERT_MSG(Size * Count <= Capacity * sizeof(T), "Block is too small to fit requested allocation.");
		}

		void Free() {}
		SizeType_t GetBlockSize() const { return Capacity * sizeof(T); }
		void* GetBlock() const { return (void*)&Block[0]; }
		bool IsValid() const { return true; }
	
	private:
		T Block[Capacity];
	};

	template<typename T, size_t InlineCount, typename TFallbackStorage = BlockStorage>
	struct SmallBlockStorage
	{
		SmallBlockStorage() :
			Block((void*)&InlineBlock[0]),
			BlockSize(InlineCount * sizeof(T))
		{}

		SmallBlockStorage(IAllocator* InPolyAllocator) :
			FallbackAllocator(InPolyAllocator),
			Block((void*)&InlineBlock[0]),
			BlockSize(InlineCount * sizeof(T))
		{}

		SmallBlockStorage(SmallBlockStorage&& Allocator) :
			Block((void*)&InlineBlock[0]),
			BlockSize(InlineCount * sizeof(T))
		{
			FallbackAllocator = bit::Move(Allocator.FallbackAllocator);
			if (Allocator.IsUsingInlineBlock())
			{
				bit::Memcpy(InlineBlock, Allocator.InlineBlock, InlineCount * sizeof(T));
			}
			else
			{
				Block = Allocator.Block;
				BlockSize = Allocator.BlockSize;
			}
		
			Allocator.Block = nullptr;
			Allocator.BlockSize = 0;
		}

		SmallBlockStorage(const SmallBlockStorage& InAllocator) :
			Block((void*)&InlineBlock[0]),
			BlockSize(InlineCount * sizeof(T))
		{
			FallbackAllocator = InAllocator.FallbackAllocator;
			if (InAllocator.IsUsingInlineBlock())
			{
				bit::Memcpy(InlineBlock, InAllocator.InlineBlock, InlineCount * sizeof(T));
			}
			else
			{
				Allocate(InAllocator.GetBlockSize(), 1);
				bit::Memcpy(Block, InAllocator.GetBlock(), InAllocator.GetBlockSize());
			}
		}

		SmallBlockStorage& operator=(SmallBlockStorage&& Allocator)
		{
			Free();
			FallbackAllocator = bit::Move(Allocator.FallbackAllocator);
			if (Allocator.IsUsingInlineBlock())
			{
				bit::Memcpy(InlineBlock, Allocator.InlineBlock, InlineCount * sizeof(T));
			}
			else
			{
				Block = Allocator.Block;
				BlockSize = Allocator.BlockSize;
			}
			Allocator.Block = nullptr;
			Allocator.BlockSize = 0;
			return *this;
		}

		SmallBlockStorage& operator=(const SmallBlockStorage& InAllocator)
		{
			Free();
			FallbackAllocator = InAllocator.FallbackAllocator;
			if (InAllocator.IsUsingInlineBlock())
			{
				bit::Memcpy(InlineBlock, InAllocator.InlineBlock, InlineCount * sizeof(T));
			}
			else
			{
				Allocate(InAllocator.GetBlockSize(), 1);
				bit::Memcpy(Block, InAllocator.GetBlock(), InAllocator.GetBlockSize());
			}
			return *this;
		}

		bool IsUsingInlineBlock() const { return Block == &InlineBlock[0]; }

		void Allocate(SizeType_t Size, SizeType_t Count)
		{
			if (Size * Count > InlineCount * sizeof(T))
			{
				FallbackAllocator.Allocate(Size, Count);
				if (IsUsingInlineBlock())
				{
					bit::Memcpy(FallbackAllocator.GetBlock(), Block, InlineCount * sizeof(T));
				}
				Block = FallbackAllocator.GetBlock();
				BlockSize = Size * Count;
			}
			else
			{
				Block = (void*)&InlineBlock[0];
				BlockSize = InlineCount * sizeof(T);
			}
		}

		void Free() 
		{
			if (!IsUsingInlineBlock())
			{
				FallbackAllocator.Free();
				Block = nullptr;
				BlockSize = 0;
			}
		}
		SizeType_t GetBlockSize() const { return IsUsingInlineBlock() ? BlockSize : FallbackAllocator.GetBlockSize(); }
		void* GetBlock() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:

		T InlineBlock[InlineCount];
		TFallbackStorage FallbackAllocator;
		void* Block;
		SizeType_t BlockSize;
	};

	struct BITLIB_API LinkedListStorage
	{
		struct BITLIB_API StorageBlockLink
		{
			StorageBlockLink* Next;
		};

		LinkedListStorage(size_t InAllocSize, size_t InAllocCount, size_t InAlignment) :
			Allocator(&bit::GetGlobalAllocator()),
			BlockList(nullptr),
			BlockData(nullptr),
			BlockUsed(0),
			AllocSize(InAllocSize),
			AllocCount(InAllocCount),
			AllocAlignment(InAlignment)
		{}

		LinkedListStorage(size_t InAllocSize, size_t InAllocCount, size_t InAlignment, IAllocator* InAllocator) :
			Allocator(InAllocator),
			BlockList(nullptr),
			BlockData(nullptr),
			BlockUsed(0),
			AllocSize(InAllocSize),
			AllocCount(InAllocCount),
			AllocAlignment(InAlignment)
		{}

		LinkedListStorage(LinkedListStorage&& Other) noexcept :
			Allocator(Other.Allocator),
			BlockList(Other.BlockList),
			BlockData(Other.BlockData),
			BlockUsed(Other.BlockUsed),
			AllocSize(Other.AllocSize),
			AllocCount(Other.AllocCount),
			AllocAlignment(Other.AllocAlignment)
		{
			Other.Allocator = nullptr;
			Other.BlockList = nullptr;
			Other.AllocSize = 0;
			Other.AllocCount = 0;
			Other.AllocAlignment = 0;
		}

		void Free()
		{
			for (StorageBlockLink* Block = BlockList; Block != nullptr; )
			{
				StorageBlockLink* Next = Block->Next;
				Allocator->Free(Block);
				Block = Next;
			}
		}

		void* AllocateLink()
		{
			/*if (Allocator == nullptr) return nullptr;
			if (BlockList == nullptr)
			{
				AllocateNewBlock();
			}
			
			if (BlockUsed + AllocSize > (AllocSize * AllocCount))
			{
				AllocateNewBlock();
			}
			void* Ptr = bit::OffsetPtr(BlockData, BlockUsed);
			BlockUsed += AllocSize;
			return Ptr;*/
			return Allocator->Allocate(AllocSize, AllocAlignment);
		}

		void FreeLink(void* Link)
		{
			Allocator->Free(Link);
		}

	private:
		void AllocateNewBlock()
		{
			StorageBlockLink* NewBlock = (StorageBlockLink*)Allocator->Allocate(sizeof(StorageBlockLink) + AllocSize * AllocCount, alignof(StorageBlockLink));
			NewBlock->Next = BlockList;
			BlockList = NewBlock;
			BlockData = bit::AlignPtr(bit::OffsetPtr(NewBlock, sizeof(StorageBlockLink)), AllocAlignment);
			BlockUsed = 0;
		}

		IAllocator* Allocator;
		StorageBlockLink* BlockList;
		void* BlockData;
		size_t BlockUsed;
		size_t AllocSize;
		size_t AllocCount;
		size_t AllocAlignment;
	};

	struct BITLIB_API HashTableStorage
	{
		HashTableStorage() :
			Allocator(&bit::GetGlobalAllocator())
		{}

		HashTableStorage(IAllocator& InAllocator) :
			Allocator(&InAllocator)
		{}

		void* AllocateBuckets(size_t Size, size_t Count)
		{
			return Allocator->Allocate(Size * Count, bit::DEFAULT_ALIGNMENT);
		}

		void FreeBuckets(void* Buckets)
		{
			Allocator->Free(Buckets);
		}

		IAllocator& GetAllocator() { return *Allocator; }

	private:
		IAllocator* Allocator;
	};

}