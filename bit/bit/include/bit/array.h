#pragma once

#include <bit/allocator.h>
#include <bit/memory.h>
#include <bit/utils.h>

namespace bit
{
	template<typename T>
	struct Array
	{
		using SizeType = int32_t;
		using SelfType = Array<T>;

		Array(bit::IAllocator& Allocator = bit::GetDefaultAllocator()) :
			Allocator(&Allocator),
			Data(nullptr),
			Count(0),
			Capacity(0)
		{}
		Array(SizeType InitialCapacity, bit::IAllocator& Allocator = bit::GetDefaultAllocator()) :
			Allocator(&Allocator),
			Data(nullptr),
			Count(0),
			Capacity(0)
		{
			Reserve(InitialCapacity);
		}
		Array(const SelfType& Copy)
		{
			Reserve(Copy.GetCount());
			Allocator = Copy.Allocator;
			bit::Memcpy(Data, Copy.GetData(), Copy.GetCount());
			Count = Copy.Count;
		}
		Array(SelfType&& Move)
		{
			Allocator = Move.Allocator;
			Data = Move.Data;
			Count = Move.Count;
			Capacity = Move.Capacity;
			Move.Invalidate();
		}
		~Array()
		{
			Destroy();
		}

		void Destroy()
		{
			if (Data != nullptr)
			{
				bit::Destroy(Data, Count);
				Allocator->Free(Data);
			}
			Invalidate();
		}

		void Invalidate()
		{
			Count = 0;
			Capacity = 0;
			Allocator = nullptr;
			Data = nullptr;
		}

		void Reserve(SizeType NewSize)
		{
			Data = static_cast<T*>(Allocator->Realloc(Data, NewSize * sizeof(T), bit::DEAFULT_ALIGNMENT));
			Capacity = NewSize;
		}

		bool Contains(const T& Element)
		{
			for (SizeType Index = 0; Index < Count; ++Index)
			{
				if (Data[Index] == Element)
				{
					return true;
				}
			}
			return false;
		}
		void Add(const T& Element)
		{
			CheckGrow();
			Data[Count++] = Element;
		}
		void Add(T&& Element)
		{
			CheckGrow();
			Data[Count++] = bit::Move(Element);
		}
		void Add(const T* Buffer, SizeType BufferCount)
		{
			CheckGrow(BufferCount);
			bit::Memcpy(&Data[Count], Buffer, sizeof(T) * BufferCount);
			Count += BufferCount;
		}
		void AddCopy(const SelfType& Other)
		{
			Add(Other.GetData(), Other.GetCount());
		}
		T& At(SizeType Index)
		{
			return Data[Index];
		}
		T& GetLast()
		{
			return Data[Count - 1];
		}
		T& operator[](SizeType Index)
		{
			return Data[Index];
		}
		SelfType& operator=(const SelfType& Copy)
		{
			bit::Destroy(Data, Count);
			CheckGrow(Copy.GetCount());
			bit::Memcpy(Data, Copy.GetData(), Copy.GetCount());
			Count = Copy.Count;
			return *this;
		}
		SelfType& operator=(SelfType&& Move)
		{
			Destroy();
			Allocator = Move.Allocator;
			Data = Move.Data;
			Count = Move.Count;
			Capacity = Move.Capacity;
			Move.Invalidate();
			return *this;
		}
		void PopLast()
		{
			if (Count > 0)
			{
				Count -= 1;
			}
		}

		template<typename TFindFunc>
		bool FindFirst(TFindFunc Func, T& Output)
		{
			for (SizeType Index = 0; Index < Count; ++Index)
			{
				if (Func(Data[Index]))
				{
					Output = Data[Index];
					return true;
				}
			}
			return false;
		}

		template<typename TFindFunc>
		bool FindAll(TFindFunc Func, SelfType& Output)
		{
			bool Result = false;
			for (SizeType Index = 0; Index < Count; ++Index)
			{
				if (Func(Data[Index]))
				{
					Output.Add(Data[Index]);
					Result = true;
				}
			}
			return Result;
		}

		template<typename TFindFunc>
		void ForEach(TFindFunc Func)
		{
			for (SizeType Index = 0; Index < Count; ++Index)
			{
				Func(Data[Index]);
			}
		}

		template<typename... TArgs>
		T* Alloc(TArgs&& ... ConstructorArgs)
		{
			CheckGrow();
			T* Ptr = &Data[Count];
			Count += 1;
			return BitPlacementNew(Ptr) T(ConstructorArgs...);
		}

		void CheckGrow(SizeType AddCount = 1)
		{
			if (!CanAdd(AddCount))
			{
			#ifdef BIT_SLOW_ARRAY_GROW
				Reserve(Capacity + AddCount);
			#else
				Reserve(Capacity * 2 + AddCount);
			#endif
			}
		}
		bool CanAdd(SizeType AddCount) { return Count + AddCount <= Capacity; }
		bool IsEmpty() const { return Count == 0; }
		void Reset() { Count = 0; }
		T* GetData() const { return Data; }
		SizeType GetCount() const { return Count; }
		SizeType GetCapacity() const { return Capacity; }
		SizeType GetCountInBytes() const { return Count * sizeof(T); }
		SizeType GetCapacityInBytes() const { return Capacity * sizeof(T); }
		bit::IAllocator& GetAllocator() { return *Allocator; }

	private:
		bit::IAllocator* Allocator;
		T* Data;
		SizeType Count;
		SizeType Capacity;
	};
}
