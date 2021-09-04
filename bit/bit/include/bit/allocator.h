#pragma once

#include <bit/types.h>

namespace bit
{
	static BIT_CONSTEXPR size_t ALLOCATOR_MAX_NAME_LEN = 128;
	struct BITLIB_API CMemoryInfo
	{
		size_t AllocatedBytes;
		size_t CommittedBytes;
		size_t ReservedBytes;
	};

	struct BITLIB_API  CMemoryArena
	{
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
		virtual CMemoryInfo GetMemoryInfo() = 0;
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
			return BitPlacementNew(Allocate<T>()) T(ConstructorArgs...);
		}

		template<typename T>
		void Delete(T* Ptr)
		{
			Ptr->~T();
			Free(Ptr);
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