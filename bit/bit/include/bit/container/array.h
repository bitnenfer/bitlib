#pragma once

#include <bit/core/os/os.h>
#include <bit/core/os/debug.h>
#include <bit/utility/utility.h>
#include <bit/core/memory/allocator.h>
#include <bit/container/storage.h>

namespace bit
{
	template<typename T>
	struct PtrFwdIterator
	{
		PtrFwdIterator(T* Ptr) : Ptr(Ptr) {}
		T& operator*() const { return *Ptr; }
		T* operator->() { return Ptr; }
		PtrFwdIterator& operator++() { Ptr++; return *this; }
		PtrFwdIterator operator++(int32_t) { PtrFwdIterator Self = *this; ++(*this); return Self; }
		friend bool operator==(const PtrFwdIterator& A, const PtrFwdIterator& B) { return A.Ptr == B.Ptr; }
		friend bool operator!=(const PtrFwdIterator& A, const PtrFwdIterator& B) { return A.Ptr != B.Ptr; }
	private:
		T* Ptr;
	};

	template<typename T>
	struct ConstPtrFwdIterator
	{
		ConstPtrFwdIterator(const T* Ptr) : Ptr(Ptr) {}
		const T& operator*() const { return *Ptr; }
		const T* operator->() const { return Ptr; }
		ConstPtrFwdIterator& operator++() { Ptr++; return *this; }
		ConstPtrFwdIterator operator++(int32_t) { ConstPtrFwdIterator Self = *this; ++(*this); return Self; }
		friend bool operator==(const ConstPtrFwdIterator& A, const ConstPtrFwdIterator& B) { return A.Ptr == B.Ptr; }
		friend bool operator!=(const ConstPtrFwdIterator& A, const ConstPtrFwdIterator& B) { return A.Ptr != B.Ptr; }
	private:
		const T* Ptr;
	};

	template<
		typename T, 
		typename TStorage = BlockStorage
	>
	struct Array
	{

		typedef Array<T, TStorage> SelfType_t;
		typedef T ElementType_t;

		/* Begin range for loop implementation */
		PtrFwdIterator<T> begin() { return PtrFwdIterator<T>(GetData()); }
		PtrFwdIterator<T> end() { return PtrFwdIterator<T>(GetData(Count)); }
		ConstPtrFwdIterator<T> cbegin() const { return ConstPtrFwdIterator<T>(GetData()); }
		ConstPtrFwdIterator<T> cend() const { return ConstPtrFwdIterator<T>(GetData(Count)); }
		/* End range for loop implementation */

		Array() : 
			Count(0), 
			Capacity(Storage.GetAllocationSize() / sizeof(T)),
			Data((T*)Storage.GetAllocation())
		{}

		Array(IAllocator& BackingAllocator) :
			Storage(&BackingAllocator),
			Count(0),
			Capacity(Storage.GetAllocationSize() / sizeof(T)),
			Data((T*)Storage.GetAllocation())
		{}

		Array(SizeType_t InitialCapacity) : 
			Count(0),
			Capacity(Storage.GetAllocationSize() / sizeof(T)),
			Data((T*)Storage.GetAllocation())
		{
			Resize(InitialCapacity);
		}

		Array(IAllocator& BackingAllocator, SizeType_t InitialCapacity) :
			Storage(&BackingAllocator),
			Count(0),
			Capacity(Storage.GetAllocationSize() / sizeof(T)),
			Data((T*)Storage.GetAllocation())
		{
			Resize(InitialCapacity);
		}

		Array(const SelfType_t& Copy) :
			Storage(Copy.Storage),
			Count(Copy.Count),
			Capacity(Storage.GetAllocationSize() / sizeof(T)),
			Data((T*)Storage.GetAllocation())
		{
		}

		Array(SelfType_t&& Move) :
			Count(0),
			Capacity(Storage.GetAllocationSize() / sizeof(T)),
			Data((T*)Storage.GetAllocation())
		{
			Storage = bit::Move(Move.Storage);
			Data = Move.Data;
			Count = Move.Count;
			Capacity = Move.Capacity;
			Move.Count = 0;
			Move.Capacity = 0;
			Move.Data = nullptr;
		}

		~Array()
		{
			Destroy();
		}

		BIT_FORCEINLINE T& At(SizeType_t Index)
		{
			BIT_ASSERT_MSG(Count > 0 && Index < Count, "Index out of bounds. Index = %d. Count = %d", Index, Count);
			return GetData()[Index];
		}

		T& GetLast() { return At(Count - 1); }

		T& GetFirst() { return At(0); }

		T& operator[](SizeType_t Index) { return At(Index); }

		SizeType_t GetCount() const { return Count; }

		SizeType_t GetCountInBytes() const { return Count * sizeof(T); }

		SizeType_t GetCapacity() const { return	Capacity; }

		SizeType_t GetCapacityInBytes() const { return Capacity * sizeof(T); }

		bool CanAdd(SizeType_t AddCount) { return Count + AddCount <= GetCapacity(); }

		bool IsEmpty() const { return Count == 0; }

		void Reset() { Count = 0; }

		bool IsValid() { return Storage.IsValid(); }

		void Destroy()
		{
			if (IsValid())
			{
				bit::DestroyArray(GetData(), Count);
				Storage.Free();
				Reset();
			}
		}

		/* We can either grow or shrink the internal storage for the array */
		void Resize(SizeType_t NewSize)
		{
			Storage.Allocate(sizeof(T), NewSize);
			BIT_ASSERT(Storage.IsValid());
			Data = (T*)Storage.GetAllocation();
			Capacity = NewSize;
		}

		void Compact()
		{
			Resize(Count);
		}

		void Add(const T& Element)
		{
			CheckGrow();
			GetData()[Count++] = Element;
		}

		void Add(T&& Element)
		{
			CheckGrow();
			GetData()[Count++] = bit::Move(Element);
		}

		void Add(const T* Buffer, SizeType_t BufferCount)
		{
			CheckGrow(BufferCount);
			bit::Memcpy(&GetData()[Count], Buffer, sizeof(T) * BufferCount);
			Count += BufferCount;
		}

		void Add(const SelfType_t& Other)
		{
			CheckGrow();
			Add(Other.GetData(), Other.GetCount());
		}

		T& AddEmpty()
		{
			CheckGrow();
			T DefaultElement{};
			Add(DefaultElement);
			return GetLast();
		}

		template<typename... TArgs>
		T& Allocate(TArgs&& ... ConstructorArgs)
		{
			CheckGrow();
			T* Ptr = &GetData()[Count++];
			bit::Construct(Ptr, bit::Forward<TArgs>(ConstructorArgs)...);
			return *Ptr;
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

		template<typename TFindFunc>
		void ForEach(TFindFunc Func)
		{
			for (T& Value : *this)
			{
				Func(Value);
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

		template<typename TFindFunc, typename TArrayType>
		bool FindAll(TFindFunc Func, TArrayType& Output)
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

		void PopLast()
		{
			if (Count > 0) Count -= 1;
		}

		SelfType_t& operator=(const SelfType_t& Copy)
		{
			bit::DestroyArray(GetData(), Count);
			Storage = Copy.Storage;
			Count = Copy.Count;
			return *this;
		}

		SelfType_t& operator=(SelfType_t&& Move)
		{
			Destroy();
			Storage = bit::Move(Move.Storage);
			Data = Move.Data;
			Count = Move.Count;
			Capacity = Move.Capacity;
			Move.Data = nullptr;
			Move.Count = 0;;
			Move.Capacity = 0;
			return *this;
		}

		void CheckGrow(SizeType_t AddCount = 1)
		{
			if (!CanAdd(AddCount))
			{
			#ifdef BIT_SLOW_ARRAY_GROW
				Reserve((Storage.GetAllocationSize() / sizeof(T)) + AddCount);
			#else
				Resize((Storage.GetAllocationSize() / sizeof(T)) * 2 + AddCount);
			#endif
			}
		}

		T* GetData() const { return Data; }

		T* GetData(SizeType_t Offset) const { return Data + Offset; }

		/* This will invalidate addresses. If you have an element by pointer or reference it won't be valid anymore */
		bool RemoveAt(SizeType_t InIndex)
		{
			if (Count > 0 && InIndex < Count)
			{
				SizeType_t Diff = Count - InIndex;
				if (Diff > 1)
				{
					bit::Memcpy(&Data[InIndex], &Data[InIndex + 1], Diff * sizeof(T));
				}
				Count -= 1;
				return true;
			}
			return false;
		}

		template<typename TSearchFunc>
		bool Remove(TSearchFunc Func)
		{
			for (SizeType_t Index = Count - 1; Index >= 0; Index--)
			{
				if (Func(Data[Index]))
				{
					return RemoveAt(Index);
				}
			}
			return false;
		}

		template<typename TSearchFunc>
		bool RemoveAll(TSearchFunc Func)
		{
			bool bFound = false;
			for (SizeType_t Index = Count - 1; Index >= 0; Index--)
			{
				if (Func(Data[Index]))
				{
					RemoveAt(Index);
					bFound = true;
				}
			}
			return bFound;
		}

	protected:
		TStorage Storage;
		SizeType_t Count;
		SizeType_t Capacity;
		T* Data;
	};
}
