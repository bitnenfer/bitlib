#pragma once

#include <bit/memory.h>
#include <bit/os.h>

namespace bit
{
	typedef int32_t DefaultContainerSizeType_t;

	template<typename TSizeType = DefaultContainerSizeType_t>
	struct TDefaultHeapAllocator
	{
		void* Allocate(void* Original, TSizeType Size, TSizeType Count)
		{
			return bit::Realloc(Original, Size * Count, DEFAULT_ALIGNMENT);
		}

		void Free(void* Ptr)
		{
			bit::Free(Ptr);
		}
	};

	template<typename TSizeType = DefaultContainerSizeType_t>
	struct TDefaultBlockAllocator
	{
		TDefaultBlockAllocator(TDefaultBlockAllocator&& Allocator) :
			Block(Allocator.Block),
			AllocationSize(Allocator.AllocationSize)
		{
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
		}
		TDefaultBlockAllocator& operator=(TDefaultBlockAllocator&& Allocator)
		{
			Block = Allocator.Block;
			AllocationSize = Allocator.AllocationSize;
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
			return *this; 
		}

		TDefaultBlockAllocator() :
			Block(nullptr),
			AllocationSize(0)
		{}

		~TDefaultBlockAllocator()
		{
			Free();
		}

		void Allocate(TSizeType Size, TSizeType Count)
		{
			Block = HeapAllocator.Allocate(Block, Size, Count);
			AllocationSize = Size * Count;
		}

		void Free()
		{
			if (IsValid())
			{
				HeapAllocator.Free(Block);
				Block = nullptr;
				AllocationSize = 0;
			}
		}

		TSizeType GetAllocationSize() const { return AllocationSize; }
		void* GetAllocation() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:
		TDefaultBlockAllocator(const TDefaultBlockAllocator&) = delete;
		void* Block;
		TSizeType AllocationSize;
		TDefaultHeapAllocator<TSizeType> HeapAllocator;
	};

	template<typename T, size_t Capacity, typename TSizeType = DefaultContainerSizeType_t>
	struct TFixedBlockAllocator
	{
		void Allocate(TSizeType Size, TSizeType Count)
		{
			BIT_ASSERT_MSG(Size * Count <= Capacity * sizeof(T), "Block is too small to fit requested allocation.");
		}

		void Free() {}
		TSizeType GetAllocationSize() const { return Capacity * sizeof(T); }
		void* GetAllocation() const { return (void*)&Block[0]; }
		bool IsValid() const { return true; }
	
	private:
		T Block[Capacity];
	};

	template<typename T, size_t InlineCount, typename TFallbackAllocator, typename TSizeType = DefaultContainerSizeType_t>
	struct TInlineBlockAllocator
	{
		TInlineBlockAllocator() :
			Block((void*)&InlineBlock[0]),
			AllocationSize(InlineCount * sizeof(T))
		{}
		TInlineBlockAllocator(TInlineBlockAllocator&& Allocator) :
			Block((void*)&InlineBlock[0]),
			AllocationSize(InlineCount * sizeof(T))
		{
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
		TInlineBlockAllocator& operator=(TInlineBlockAllocator&& Allocator)
		{
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

		bool IsUsingInlineBlock() const { return Block == &InlineBlock[0]; }

		void Allocate(TSizeType Size, TSizeType Count)
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
		TSizeType GetAllocationSize() const { return AllocationSize; }
		void* GetAllocation() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:
		TInlineBlockAllocator(const TInlineBlockAllocator&) = delete;

		T InlineBlock[InlineCount];
		TFallbackAllocator FallbackAllocator;
		void* Block;
		TSizeType AllocationSize;
	};

	template<typename T, size_t InlineCount, typename TSizeType = DefaultContainerSizeType_t>
	using TDefaultInlineBlockAllocator = TInlineBlockAllocator<T, InlineCount, TDefaultBlockAllocator<TSizeType>, TSizeType>;

	template<typename TSizeType = DefaultContainerSizeType_t>
	struct TDefaultLinkedListAllocator
	{
		template<typename TLinkType>
		struct TLinkAllocator
		{
			TLinkType* AllocateLink(TSizeType Count)
			{
				return (TLinkType*)HeapAllocator.Allocate(nullptr, sizeof(TLinkType), 1);
			}

			void FreeLink(TLinkType* Link)
			{
				HeapAllocator.Free(Link);
			}

			TDefaultHeapAllocator<TSizeType> HeapAllocator;
		};
	};

	template<typename TSizeType = DefaultContainerSizeType_t>
	struct TDefaultHashTableAllocator
	{
		typedef TDefaultLinkedListAllocator<TSizeType> BucketEntryAllocatorType_t;
		typedef TDefaultHeapAllocator<TSizeType> BucketAllocatorType_t;
	};
}