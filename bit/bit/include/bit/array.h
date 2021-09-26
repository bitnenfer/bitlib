#pragma once

#include <bit/os.h>
#include <bit/utils.h>
#include <bit/allocator.h>
#include <bit/container_allocators.h>

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
		typename TAllocator = DefaultBlockAllocator
	>
	struct Array
	{

		typedef Array<T, TAllocator> SelfType_t;
		typedef T ElementType_t;

		/* Begin range for loop implementation */
		PtrFwdIterator<T> begin() { return PtrFwdIterator<T>(GetData()); }
		PtrFwdIterator<T> end() { return PtrFwdIterator<T>(GetData(Count)); }
		ConstPtrFwdIterator<T> cbegin() const { return ConstPtrFwdIterator<T>(GetData()); }
		ConstPtrFwdIterator<T> cend() const { return ConstPtrFwdIterator<T>(GetData(Count)); }
		/* End range for loop implementation */

		Array() : 
			Count(0), 
			Capacity(Allocator.GetAllocationSize() / sizeof(T)),
			Data((T*)Allocator.GetAllocation())
		{}

		Array(SizeType_t InitialCapacity) : 
			Count(0),
			Capacity(Allocator.GetAllocationSize() / sizeof(T)),
			Data((T*)Allocator.GetAllocation())
		{
			Resize(InitialCapacity);
		}

		Array(const SelfType_t& Copy) :
			Count(0),
			Capacity(Allocator.GetAllocationSize() / sizeof(T)),
			Data((T*)Allocator.GetAllocation())
		{
			Resize(Copy.GetCount());
			bit::Memcpy(GetData(), Copy.GetData(), Copy.GetCount());
			Count = Copy.Count;
			Capacity = Copy.Capacity;
		}

		Array(SelfType_t&& Move) :
			Count(0),
			Capacity(Allocator.GetAllocationSize() / sizeof(T)),
			Data((T*)Allocator.GetAllocation())
		{
			Allocator = bit::Move(Move.Allocator);
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

		bool IsValid() { return Allocator.IsValid(); }

		void Destroy()
		{
			if (IsValid())
			{
				bit::DestroyArray(GetData(), Count);
				Allocator.Free();
				Reset();
			}
		}

		/* We can either grow or shrink the internal storage for the array */
		void Resize(SizeType_t NewSize)
		{
			Allocator.Allocate(sizeof(T), NewSize);
			BIT_ASSERT(Allocator.IsValid());
			Data = (T*)Allocator.GetAllocation();
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
			CheckGrow(Copy.GetCount());
			bit::Memcpy(GetData(), Copy.GetData(), Copy.GetCount());
			Count = Copy.Count;
			return *this;
		}

		SelfType_t& operator=(SelfType_t&& Move)
		{
			Destroy();
			Allocator = bit::Move(Move.Allocator);
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
				Reserve((Allocator.GetAllocationSize() / sizeof(T)) + AddCount);
			#else
				Resize((Allocator.GetAllocationSize() / sizeof(T)) * 2 + AddCount);
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
		TAllocator Allocator;
		SizeType_t Count;
		SizeType_t Capacity;
		T* Data;
	};

	namespace pmr
	{
		template<
			typename T
		>
		struct Array
		{
			typedef Array<T> SelfType_t;
			typedef T ElementType_t;

			/* Begin range for loop implementation */
			PtrFwdIterator<T> begin() { return PtrFwdIterator<T>(GetData()); }
			PtrFwdIterator<T> end() { return PtrFwdIterator<T>(GetData(Count)); }
			ConstPtrFwdIterator<T> cbegin() const { return ConstPtrFwdIterator<T>(GetData()); }
			ConstPtrFwdIterator<T> cend() const { return ConstPtrFwdIterator<T>(GetData(Count)); }
			/* End range for loop implementation */

			Array(Allocator& Allocator) :
				Allocator(&Allocator),
				Data(nullptr),
				Count(0),
				Capacity(0)
			{}

			Array(SizeType_t InitialCapacity, Allocator& Allocator) :
				Allocator(&Allocator),
				Data(nullptr),
				Count(0),
				Capacity(0)
			{
				Resize(InitialCapacity);
			}

			Array(const SelfType_t& Copy) :
				Allocator(nullptr),
				Data(nullptr),
				Count(0),
				Capacity(0)
			{
				Allocator = Copy.Allocator;
				Resize(Copy.GetCount());
				bit::Memcpy(Data, Copy.GetData(), Copy.GetCount());
				Count = Copy.Count;
			}
			
			Array(SelfType_t&& Move) :
				Allocator(nullptr),
				Data(nullptr),
				Count(0),
				Capacity(0)
			{
				Allocator = Move.Allocator;
				Data = Move.GetData();
				Count = Move.Count;
				Capacity = Move.Capacity;
				Move.Invalidate();
			}
			
			~Array()
			{
				Destroy();
			}

			bool IsValid() const 
			{
				return Data != nullptr;
			}

			void Invalidate()
			{
				Allocator = nullptr;
				Data = nullptr;
				Count = 0;
				Capacity = 0;
			}

			void Destroy()
			{
				if (IsValid())
				{
					bit::DestroyArray(Data, Count);
					if (Allocator != nullptr)
					{
						Allocator->Free(Data);
					}
					Invalidate();
				}
			}

			void Resize(SizeType_t NewSize)
			{
				Data = (T*)Allocator->Reallocate(Data, NewSize * sizeof(T), alignof(T));
				Capacity = NewSize;
				BIT_ASSERT(Data != nullptr);
			}

			void Compact()
			{
				Resize(Count);
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

			SizeType_t GetCapacity() const { return Capacity; }

			SizeType_t GetCapacityInBytes() const { return Capacity * sizeof(T); }

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
				T DefaultElement;
				Add(DefaultElement);
				return GetLast();
			}

			template<typename... TArgs>
			T& New(TArgs&& ... ConstructorArgs)
			{
				CheckGrow();
				T* Ptr = &GetData()[Count++];
				bit::Construct(Ptr, bit::Forward<TArgs>(ConstructorArgs)...);
				return Ptr;
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

			void CheckGrow(SizeType_t AddCount = 1)
			{
				if (!CanAdd(AddCount))
				{
	#ifdef BIT_SLOW_ARRAY_GROW
					Reserve(Capacity + AddCount);
	#else
					Resize(Capacity * 2 + AddCount);
	#endif
				}
			}

			T* GetData() const { return Data; }

			T* GetData(SizeType_t Offset) const { return Data + Offset; }

			bool CanAdd(SizeType_t AddCount) { return Count + AddCount <= GetCapacity(); }

			Allocator& GetAllocator() { return *Allocator; }

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
			bit::Allocator* Allocator;
			T* Data;
			SizeType_t Count;
			SizeType_t Capacity;
		};
	}
}
