#pragma once

#include <bit/core/memory.h>
#include <bit/core/os/debug.h>
#include <bit/utility/utility.h>

namespace bit
{
	typedef int32_t SizeType_t;
	static_assert(bit::IsSigned<SizeType_t>::Value, "Size type must be signed");

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
		BlockStorage(BlockStorage&& InBlockAllocator) :
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
			BackingAllocator(&bit::GetDefaultAllocator())
		{
			if (InAllocator.IsValid())
			{
				Allocate(InAllocator.GetAllocationSize(), 1);
				bit::Memcpy(Block, InAllocator.GetAllocation(), InAllocator.GetAllocationSize());
			}
		}

		BlockStorage(IAllocator* Allocator, const BlockStorage& InAllocator) :
			Block(nullptr),
			AllocationSize(0),
			BackingAllocator(Allocator)
		{
			if (InAllocator.IsValid())
			{
				Allocate(InAllocator.GetAllocationSize(), 1);
				bit::Memcpy(Block, InAllocator.GetAllocation(), InAllocator.GetAllocationSize());
			}
		}

		BlockStorage& operator=(const BlockStorage& InAllocator)
		{
			Free();
			if (InAllocator.IsValid())
			{
				Allocate(InAllocator.GetAllocationSize(), 1);
				bit::Memcpy(Block, InAllocator.GetAllocation(), InAllocator.GetAllocationSize());
			}
			return *this;
		}

		BlockStorage& operator=(BlockStorage&& InBlockAllocator)
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
			BackingAllocator(&bit::GetDefaultAllocator())
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

		SizeType_t GetAllocationSize() const { return AllocationSize; }
		void* GetAllocation() const { return Block; }
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
		SizeType_t GetAllocationSize() const { return Capacity * sizeof(T); }
		void* GetAllocation() const { return (void*)&Block[0]; }
		bool IsValid() const { return true; }
	
	private:
		T Block[Capacity];
	};

	template<typename T, size_t InlineCount, typename TFallbackStorage = BlockStorage>
	struct SmallBlockStorage
	{
		SmallBlockStorage() :
			Block((void*)&InlineBlock[0]),
			AllocationSize(InlineCount * sizeof(T))
		{}

		SmallBlockStorage(IAllocator* InPolyAllocator) :
			FallbackAllocator(InPolyAllocator),
			Block((void*)&InlineBlock[0]),
			AllocationSize(InlineCount * sizeof(T))
		{}

		SmallBlockStorage(SmallBlockStorage&& Allocator) :
			Block((void*)&InlineBlock[0]),
			AllocationSize(InlineCount * sizeof(T))
		{
			FallbackAllocator = bit::Move(Allocator.FallbackAllocator);
			if (Allocator.IsUsingInlineBlock())
			{
				bit::Memcpy(InlineBlock, Allocator.InlineBlock, InlineCount * sizeof(T));
			}
			else
			{
				Block = Allocator.Block;
				AllocationSize = Allocator.AllocationSize;
			}
		
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
		}

		SmallBlockStorage(const SmallBlockStorage& InAllocator) :
			Block((void*)&InlineBlock[0]),
			AllocationSize(InlineCount * sizeof(T))
		{
			FallbackAllocator = InAllocator.FallbackAllocator;
			if (InAllocator.IsUsingInlineBlock())
			{
				bit::Memcpy(InlineBlock, InAllocator.InlineBlock, InlineCount * sizeof(T));
			}
			else
			{
				Allocate(InAllocator.GetAllocationSize(), 1);
				bit::Memcpy(Block, InAllocator.GetAllocation(), InAllocator.GetAllocationSize());
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
				AllocationSize = Allocator.AllocationSize;
			}
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
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
				Allocate(InAllocator.GetAllocationSize(), 1);
				bit::Memcpy(Block, InAllocator.GetAllocation(), InAllocator.GetAllocationSize());
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
					bit::Memcpy(FallbackAllocator.GetAllocation(), Block, InlineCount * sizeof(T));
				}
				Block = FallbackAllocator.GetAllocation();
				AllocationSize = Size * Count;
			}
			else
			{
				Block = (void*)&InlineBlock[0];
				AllocationSize = InlineCount * sizeof(T);
			}
		}

		void Free() 
		{
			if (!IsUsingInlineBlock())
			{
				FallbackAllocator.Free();
				Block = nullptr;
				AllocationSize = 0;
			}
		}
		SizeType_t GetAllocationSize() const { return AllocationSize; }
		void* GetAllocation() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:

		T InlineBlock[InlineCount];
		TFallbackStorage FallbackAllocator;
		void* Block;
		SizeType_t AllocationSize;
	};

	struct BITLIB_API LinkedListStorage
	{
		template<typename TLinkType>
		struct LinkAllocator
		{
			LinkAllocator() :
				Allocator(&bit::GetDefaultAllocator())
			{}

			LinkAllocator(IAllocator* InAllocator) :
				Allocator(InAllocator)
			{}

			TLinkType* AllocateLink(SizeType_t Count)
			{
				return (TLinkType*)Allocator->Reallocate(nullptr, sizeof(TLinkType), bit::DEFAULT_ALIGNMENT);
			}

			void FreeLink(TLinkType* Link)
			{
				Allocator->Free(Link);
			}

			IAllocator* Allocator;
		};
	};

	struct BITLIB_API HashTableStorage
	{
		typedef LinkedListStorage BucketEntryAllocatorType_t;
		typedef DefaultHeapAllocator BucketAllocatorType_t;
	};
}