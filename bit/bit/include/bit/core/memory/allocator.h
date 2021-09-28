#pragma once

#include <bit/core/types.h>
#include <bit/utility/utility.h>
#include <bit/utility/reference_counter.h>

namespace bit
{
	static constexpr size_t ALLOCATOR_MAX_NAME_LEN = 128;
	struct BITLIB_API MemoryUsageInfo
	{
		size_t AllocatedBytes;
		size_t CommittedBytes;
		size_t ReservedBytes;
	};

	struct IAllocator;

	struct BITLIB_API MemoryArena
	{
		typedef AtomicRefCounter<int64_t> RefCounter_t;

		MemoryArena() : 
			RefCounter(nullptr),
			Allocator(nullptr),
			BaseAddress(nullptr), 
			SizeInBytes(0) 
		{}
		
		MemoryArena(RefCounter_t* RefCounter, IAllocator* Allocator, void* BaseAddress, size_t SizeInBytes) :
			RefCounter(RefCounter),
			Allocator(Allocator),
			BaseAddress(BaseAddress), 
			SizeInBytes(SizeInBytes) 
		{
		}

		MemoryArena(void* BaseAddress, size_t SizeInBytes) :
			RefCounter(nullptr),
			Allocator(nullptr),
			BaseAddress(BaseAddress),
			SizeInBytes(SizeInBytes)
		{
		}

		MemoryArena(const MemoryArena& Copy)
		{
			RefCounter = Copy.RefCounter;
			Allocator = Copy.Allocator;
			BaseAddress = Copy.BaseAddress;
			SizeInBytes = Copy.SizeInBytes;
			if (RefCounter != nullptr) RefCounter->Increment();
		}

		MemoryArena& operator=(const MemoryArena& Copy)
		{
			RefCounter = Copy.RefCounter;
			Allocator = Copy.Allocator;
			BaseAddress = Copy.BaseAddress;
			SizeInBytes = Copy.SizeInBytes;
			if (RefCounter != nullptr) RefCounter->Increment();
			return *this;
		}

		MemoryArena(MemoryArena&& Move)
		{
			RefCounter = Move.RefCounter;
			Allocator = Move.Allocator;
			BaseAddress = Move.BaseAddress;
			SizeInBytes = Move.SizeInBytes;
			Move.RefCounter = nullptr;
			Move.Allocator = nullptr;
			Move.BaseAddress = nullptr;
			Move.SizeInBytes = 0;
		}

		MemoryArena& operator=(MemoryArena&& Move)
		{
			RefCounter = Move.RefCounter;
			Allocator = Move.Allocator;
			BaseAddress = Move.BaseAddress;
			SizeInBytes = Move.SizeInBytes;
			Move.RefCounter = nullptr;
			Move.Allocator = nullptr;
			Move.BaseAddress = nullptr;
			Move.SizeInBytes = 0;
			return *this;
		}

		~MemoryArena();
		
		void* GetBaseAddress() const { return BaseAddress; }
		void* GetEndAddress() const { return bit::OffsetPtr(BaseAddress, SizeInBytes); }
		size_t GetSizeInBytes() const { return SizeInBytes; }
		bool OwnsAllocation(const void* Ptr) const { return bit::PtrInRange(Ptr, GetBaseAddress(), GetEndAddress()); }

	private:
		RefCounter_t* RefCounter;
		IAllocator* Allocator;
		void* BaseAddress;
		size_t SizeInBytes;
	};

	template<size_t CapacityInBytes>
	struct FixedMemoryArena : public MemoryArena
	{
		FixedMemoryArena() :
			MemoryArena(&Data[0], CapacityInBytes)
		{}

		uint8_t Data[CapacityInBytes];
	};

	/* Polymorphic IAllocator. Useful for custom types of allocations and Polymorphic containers. */
	struct BITLIB_API IAllocator
	{
		IAllocator(const char* Name);

		virtual void* Allocate(size_t Size, size_t Alignment) = 0;
		virtual void* Reallocate(void* Pointer, size_t Size, size_t Alignment) = 0;
		virtual void Free(void* Pointer) = 0;
		virtual size_t GetSize(void* Pointer) = 0;
		virtual MemoryUsageInfo GetMemoryUsageInfo() = 0;
		virtual bool CanAllocate(size_t Size, size_t Alignment) = 0;
		virtual bool OwnsAllocation(const void* Ptr) = 0;
		virtual size_t Compact() { return 0; }

		const char* GetName() const { return Name; }

		template<typename T>
		T* Allocate()
		{
			return (T*)Allocate(sizeof(T), alignof(T));
		}

		template<typename T>
		T* AllocateArray(size_t Count)
		{
			return (T*)Allocate(sizeof(T) * Count, alignof(T));
		}

		template<typename T, typename... TArgs>
		T* New(TArgs&& ... ConstructorArgs)
		{
			return BitPlacementNew(Allocate<T>()) T(bit::Forward<TArgs>(ConstructorArgs)...);
		}

		template<typename T>
		void Delete(T* Ptr)
		{
			Ptr->~T();
			Free(Ptr);
		}

		MemoryArena AllocateArena(size_t Size, size_t Alignment = bit::DEFAULT_ALIGNMENT)
		{
			return MemoryArena(New<MemoryArena::RefCounter_t>(1), this, Allocate(Size, Alignment), Size);
		}

	private:
		char Name[bit::ALLOCATOR_MAX_NAME_LEN];
	};
}

/* I recommend just using IAllocator's New, Delete, NewArray and DeleteArray instead */

BIT_FORCEINLINE void* operator new(size_t Size, bit::IAllocator& Allocator)
{
	return Allocator.Allocate(Size, 0);
}

BIT_FORCEINLINE void operator delete(void* Ptr, bit::IAllocator& Allocator)
{
	Allocator.Free(Ptr);
}

BIT_FORCEINLINE void* operator new[](size_t Size, bit::IAllocator& Allocator)
{
	return Allocator.Allocate(Size, 0);
}

BIT_FORCEINLINE void operator delete[](void* Ptr, size_t Size, bit::IAllocator& Allocator)
{
	Allocator.Free(Ptr);
}