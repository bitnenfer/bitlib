#pragma once

#include <bit/types.h>
#include <bit/utils.h>
#include <bit/reference_counter.h>

namespace bit
{
	static BIT_CONSTEXPR size_t ALLOCATOR_MAX_NAME_LEN = 128;
	struct BITLIB_API CMemoryUsageInfo
	{
		size_t AllocatedBytes;
		size_t CommittedBytes;
		size_t ReservedBytes;
	};

	struct IAllocator;

	struct BITLIB_API CMemoryArena
	{
		typedef TAtomicRefCounter<int64_t> RefCounter_t;

		CMemoryArena() : 
			RefCounter(nullptr),
			Allocator(nullptr),
			BaseAddress(nullptr), 
			SizeInBytes(0) 
		{}
		
		CMemoryArena(RefCounter_t* RefCounter, IAllocator* Allocator, void* BaseAddress, size_t SizeInBytes) :
			RefCounter(RefCounter),
			Allocator(Allocator),
			BaseAddress(BaseAddress), 
			SizeInBytes(SizeInBytes) 
		{
		}

		CMemoryArena(void* BaseAddress, size_t SizeInBytes) :
			RefCounter(nullptr),
			Allocator(nullptr),
			BaseAddress(BaseAddress),
			SizeInBytes(SizeInBytes)
		{
		}

		CMemoryArena(const CMemoryArena& Copy)
		{
			RefCounter = Copy.RefCounter;
			Allocator = Copy.Allocator;
			BaseAddress = Copy.BaseAddress;
			SizeInBytes = Copy.SizeInBytes;
			if (RefCounter != nullptr) RefCounter->Increment();
		}

		CMemoryArena& operator=(const CMemoryArena& Copy)
		{
			RefCounter = Copy.RefCounter;
			Allocator = Copy.Allocator;
			BaseAddress = Copy.BaseAddress;
			SizeInBytes = Copy.SizeInBytes;
			if (RefCounter != nullptr) RefCounter->Increment();
			return *this;
		}

		CMemoryArena(CMemoryArena&& Move)
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

		CMemoryArena& operator=(CMemoryArena&& Move)
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

		~CMemoryArena();
		
		void* GetBaseAddress() const { return BaseAddress; }
		size_t GetSizeInBytes() const { return SizeInBytes; }

	private:
		RefCounter_t* RefCounter;
		IAllocator* Allocator;
		void* BaseAddress;
		size_t SizeInBytes;
	};

	/* Polymorphic Allocator. Useful for custom types of allocations and Polymorphic containers. */
	struct BITLIB_API IAllocator
	{
		IAllocator(const char* Name);

		virtual void* Allocate(size_t Size, size_t Alignment) = 0;
		virtual void* Reallocate(void* Pointer, size_t Size, size_t Alignment) = 0;
		virtual void Free(void* Pointer) = 0;
		virtual size_t GetSize(void* Pointer) = 0;
		virtual CMemoryUsageInfo GetMemoryUsageInfo() = 0;
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

		CMemoryArena AllocateArena(size_t Size, size_t Alignment = bit::DEFAULT_ALIGNMENT)
		{
			return CMemoryArena(New<CMemoryArena::RefCounter_t>(1), this, Allocate(Size, Alignment), Size);
		}

	private:
		char Name[bit::ALLOCATOR_MAX_NAME_LEN];
	};
}

/* I recommend just using allocator's New, Delete, NewArray and DeleteArray instead */

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