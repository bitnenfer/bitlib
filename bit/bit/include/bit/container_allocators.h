#pragma once

#include <bit/memory.h>
#include <bit/os.h>

namespace bit
{
	typedef int32_t SizeType_t;
	static_assert(bit::TIsSigned<SizeType_t>::Value, "Size type must be signed");

	struct BITLIB_API CDefaultHeapAllocator
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

	struct BITLIB_API CDefaultBlockAllocator
	{
		CDefaultBlockAllocator(CDefaultBlockAllocator&& Allocator) :
			Block(Allocator.Block),
			AllocationSize(Allocator.AllocationSize)
		{
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
		}
		CDefaultBlockAllocator& operator=(CDefaultBlockAllocator&& Allocator)
		{
			Block = Allocator.Block;
			AllocationSize = Allocator.AllocationSize;
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
			return *this; 
		}

		CDefaultBlockAllocator() :
			Block(nullptr),
			AllocationSize(0)
		{}

		~CDefaultBlockAllocator()
		{
			Free();
		}

		void Allocate(SizeType_t Size, SizeType_t Count)
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

		SizeType_t GetAllocationSize() const { return AllocationSize; }
		void* GetAllocation() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:
		CDefaultBlockAllocator(const CDefaultBlockAllocator&) = delete;
		void* Block;
		SizeType_t AllocationSize;
		CDefaultHeapAllocator HeapAllocator;
	};

	template<typename T, size_t Capacity>
	struct TFixedBlockAllocator
	{
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

	template<typename T, size_t InlineCount, typename TFallbackAllocator = CDefaultBlockAllocator>
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
		TInlineBlockAllocator(const TInlineBlockAllocator&) = delete;

		T InlineBlock[InlineCount];
		TFallbackAllocator FallbackAllocator;
		void* Block;
		SizeType_t AllocationSize;
	};

	struct BITLIB_API CDefaultLinkedListAllocator
	{
		template<typename TLinkType>
		struct TLinkAllocator
		{
			TLinkType* AllocateLink(SizeType_t Count)
			{
				return (TLinkType*)HeapAllocator.Allocate(nullptr, sizeof(TLinkType), 1);
			}

			void FreeLink(TLinkType* Link)
			{
				HeapAllocator.Free(Link);
			}

			CDefaultHeapAllocator HeapAllocator;
		};
	};

	struct BITLIB_API CDefaultHashTableAllocator
	{
		typedef CDefaultLinkedListAllocator BucketEntryAllocatorType_t;
		typedef CDefaultHeapAllocator BucketAllocatorType_t;
	};
}