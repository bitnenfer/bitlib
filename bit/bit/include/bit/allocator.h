#pragma once

#include <bit/types.h>

namespace bit
{
	static BIT_CONSTEXPR size_t ALLOCATOR_MAX_NAME_LEN = 128;
	struct BITLIB_API IAllocator
	{
		IAllocator(const char* Name);

		virtual void* Alloc(size_t Size, size_t Alignment) = 0;
		virtual void* Realloc(void* Pointer, size_t Size, size_t Alignment) = 0;
		virtual void Free(void* Pointer) = 0;
		virtual size_t GetSize(void* Pointer) = 0;
		virtual size_t GetTotalUsedMemory() = 0;
		const char* GetName() const { return Name; }

		template<typename T>
		T* Alloc(size_t Count = 1)
		{
			return (T*)Alloc(sizeof(T) * Count, 4);
		}

		template<typename T, typename... TArgs>
		T* New(TArgs&& ... ConstructorArgs)
		{
			return BitPlacementNew((T*)Alloc(sizeof(T), 4)) T(ConstructorArgs...);
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
