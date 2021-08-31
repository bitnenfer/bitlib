#pragma once

#include <bit/allocator.h>
#include <bit/memory.h>
#include <bit/utils.h>
#include <bit/os.h>

namespace bit
{
	template<typename T, typename TSizeType = int64_t>
	struct TArrayImpl
	{
		typedef TArrayImpl<T, TSizeType> SelfType_t;

		TArrayImpl(T* Begin, T* End) :
			Begin(Begin), End(End)
		{}

		T* GetData() const { return Begin; }
		TSizeType GetCount() const { return bit::PtrDiff((void*)Begin, (void*)End); }
		TSizeType GetCountInBytes() const { return GetCount() * sizeof(T); }

	private:
		T* Begin;
		T* End;
	};

	template<typename T, typename TSizeType = int64_t>
	struct TArray
	{
		typedef TArray<T, TSizeType> SelfType_t;

		TArray(bit::IAllocator& Allocator = bit::GetDefaultAllocator()) :
			Allocator(&Allocator),
			Data(nullptr),
			Count(0),
			Capacity(0)
		{}
		TArray(TSizeType InitialCapacity, bit::IAllocator& Allocator = bit::GetDefaultAllocator()) :
			Allocator(&Allocator),
			Data(nullptr),
			Count(0),
			Capacity(0)
		{
			Reserve(InitialCapacity);
		}
		TArray(const SelfType_t& Copy)
		{
			Reserve(Copy.GetCount());
			Allocator = Copy.Allocator;
			bit::Memcpy(Data, Copy.GetData(), Copy.GetCount());
			Count = Copy.Count;
		}
		TArray(SelfType_t&& Move)
		{
			Allocator = Move.Allocator;
			Data = Move.Data;
			Count = Move.Count;
			Capacity = Move.Capacity;
			Move.Invalidate();
		}
		~TArray()
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

		void Reserve(TSizeType NewSize)
		{
			Data = static_cast<T*>(Allocator->Realloc(Data, NewSize * sizeof(T), bit::DEFAULT_ALIGNMENT));
			Capacity = NewSize;
			BIT_ASSERT(Data != nullptr);
		}

		bool Contains(const T& Element)
		{
			for (const T& Value : *this)
			{
				if (Value == Element)
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
		void Add(const T* Buffer, TSizeType BufferCount)
		{
			CheckGrow(BufferCount);
			bit::Memcpy(&Data[Count], Buffer, sizeof(T) * BufferCount);
			Count += BufferCount;
		}
		void AddCopy(const SelfType_t& Other)
		{
			Add(Other.GetData(), Other.GetCount());
		}
		T& At(TSizeType Index)
		{
			BIT_ASSERT_MSG(Index < Count, "Array index out of bounds");
			return Data[Index];
		}
		T& GetLast()
		{
			BIT_ASSERT_MSG(Count > 1, "Array access out of bounds");
			return Data[Count - 1];
		}
		T& operator[](TSizeType Index)
		{
			BIT_ASSERT_MSG(Index < Count, "Array index out of bounds");
			return Data[Index];
		}
		SelfType_t& operator=(const SelfType_t& Copy)
		{
			bit::Destroy(Data, Count);
			CheckGrow(Copy.GetCount());
			bit::Memcpy(Data, Copy.GetData(), Copy.GetCount());
			Count = Copy.Count;
			return *this;
		}
		SelfType_t& operator=(SelfType_t&& Move)
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
			for (T& Value : *this)
			{
				if (Func(Value))
				{
					Output = Value;
					return true;
				}
			}
			return false;
		}

		template<typename TFindFunc>
		bool FindAll(TFindFunc Func, SelfType_t& Output)
		{
			bool Result = false;
			for (T& Value : *this)
			{
				if (Func(Value))
				{
					Output.Add(Value);
					Result = true;
				}
			}
			return Result;
		}

		template<typename TFindFunc>
		void ForEach(TFindFunc Func)
		{
			for (T& Value : *this)
			{
				Func(Value);
			}
		}

		template<typename... TArgs>
		T* Alloc(TArgs&& ... ConstructorArgs)
		{
			CheckGrow();
			T* Ptr = &Data[Count];
			Count += 1;
			bit::Construct(Ptr, 1, ConstructorArgs...);
			return Ptr;
		}

		void CheckGrow(TSizeType AddCount = 1)
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
		bool CanAdd(TSizeType AddCount) { return Count + AddCount <= Capacity; }
		bool IsEmpty() const { return Count == 0; }
		void Reset() { Count = 0; }
		T* GetData() const { return Data; }
		TSizeType GetCount() const { return Count; }
		TSizeType GetCapacity() const { return Capacity; }
		TSizeType GetCountInBytes() const { return Count * sizeof(T); }
		TSizeType GetCapacityInBytes() const { return Capacity * sizeof(T); }
		bit::IAllocator& GetAllocator() { return *Allocator; }

		/* Begin range for loop implementation */
		struct CIterator
		{
			CIterator(T* Ptr, TSizeType Offset) : Ptr(Ptr + Offset) {}
			T& operator*() const { return *Ptr; }
			T* operator->() { return Ptr; }
			CIterator& operator++() { Ptr++; return *this; }
			CIterator operator++(int32_t) { CIterator Self = *this; ++(*this); return Self; }
			friend bool operator==(const CIterator& A, const CIterator& B) { return A.Ptr == B.Ptr; }
			friend bool operator!=(const CIterator& A, const CIterator& B) { return A.Ptr != B.Ptr; }
		private:
			T* Ptr;
		};
		struct CConstIterator
		{
			CConstIterator(const T* Ptr, TSizeType Offset) : Ptr(Ptr + Offset) {}
			const T& operator*() const { return *Ptr; }
			const T* operator->() const { return Ptr; }
			CConstIterator& operator++() { Ptr++; return *this; }
			CConstIterator operator++(int32_t) { CConstIterator Self = *this; ++(*this); return Self; }
			friend bool operator==(const CConstIterator& A, const CConstIterator& B) { return A.Ptr == B.Ptr; }
			friend bool operator!=(const CConstIterator& A, const CConstIterator& B) { return A.Ptr != B.Ptr; }
		private:
			const T* Ptr;
		};

		CIterator begin() { return CIterator(Data, 0); }
		CIterator end() { return CIterator(Data, GetCount()); }
		CConstIterator cbegin() const { return CConstIterator(Data, 0); }
		CConstIterator cend() const { return CConstIterator(Data, GetCount()); }
		/* End range for loop implementation */

	private:
		bit::IAllocator* Allocator;
		T* Data;
		TSizeType Count;
		TSizeType Capacity;
	};
}
