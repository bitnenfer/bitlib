#pragma once

#include <bit/os.h>
#include <bit/utils.h>
#include <bit/allocator.h>
#include <bit/container_allocators.h>

namespace bit
{
	template<typename T>
	struct TPtrFwdIterator
	{
		TPtrFwdIterator(T* Ptr) : Ptr(Ptr) {}
		T& operator*() const { return *Ptr; }
		T* operator->() { return Ptr; }
		TPtrFwdIterator& operator++() { Ptr++; return *this; }
		TPtrFwdIterator operator++(int32_t) { TPtrFwdIterator Self = *this; ++(*this); return Self; }
		friend bool operator==(const TPtrFwdIterator& A, const TPtrFwdIterator& B) { return A.Ptr == B.Ptr; }
		friend bool operator!=(const TPtrFwdIterator& A, const TPtrFwdIterator& B) { return A.Ptr != B.Ptr; }
	private:
		T* Ptr;
	};

	template<typename T>
	struct TConstPtrFwdIterator
	{
		TConstPtrFwdIterator(const T* Ptr) : Ptr(Ptr) {}
		const T& operator*() const { return *Ptr; }
		const T* operator->() const { return Ptr; }
		TConstPtrFwdIterator& operator++() { Ptr++; return *this; }
		TConstPtrFwdIterator operator++(int32_t) { TConstPtrFwdIterator Self = *this; ++(*this); return Self; }
		friend bool operator==(const TConstPtrFwdIterator& A, const TConstPtrFwdIterator& B) { return A.Ptr == B.Ptr; }
		friend bool operator!=(const TConstPtrFwdIterator& A, const TConstPtrFwdIterator& B) { return A.Ptr != B.Ptr; }
	private:
		const T* Ptr;
	};

	template<
		typename T, 
		typename TSizeType = DefaultContainerSizeType_t,
		typename TAllocator = TDefaultBlockAllocator<TSizeType>
	>
	struct TArray
	{
		static_assert(bit::TIsSigned<TSizeType>::Value, "Size type must be signed");

		typedef TArray<T, TSizeType, TAllocator> SelfType_t;
		typedef T ElementType_t;
		typedef TSizeType SizeType_t;

		/* Begin range for loop implementation */
		TPtrFwdIterator<T> begin() { return TPtrFwdIterator<T>(GetData()); }
		TPtrFwdIterator<T> end() { return TPtrFwdIterator<T>(GetData(Count)); }
		TConstPtrFwdIterator<T> cbegin() const { return TConstPtrFwdIterator<T>(GetData()); }
		TConstPtrFwdIterator<T> cend() const { return TConstPtrFwdIterator<T>(GetData(Count)); }
		/* End range for loop implementation */

		TArray() : 
			Count(0), 
			Capacity(Allocator.GetAllocationSize() / sizeof(T)),
			Data((T*)Allocator.GetAllocation())
		{}

		TArray(TSizeType InitialCapacity) : 
			Count(0),
			Capacity(Allocator.GetAllocationSize() / sizeof(T)),
			Data((T*)Allocator.GetAllocation())
		{
			Reserve(InitialCapacity);
		}

		TArray(const SelfType_t& Copy) :
			Count(0),
			Capacity(Allocator.GetAllocationSize() / sizeof(T)),
			Data((T*)Allocator.GetAllocation())
		{
			Reserve(Copy.GetCount());
			bit::Memcpy(GetData(), Copy.GetData(), Copy.GetCount());
			Count = Copy.Count;
			Capacity = Copy.Capacity;
		}

		TArray(SelfType_t&& Move) :
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

		~TArray()
		{
			Destroy();
		}

		BIT_FORCEINLINE T& At(TSizeType Index)
		{
			BIT_ASSERT_MSG(Count > 0 && Index < Count, "Index out of bounds. Index = %d. Count = %d\n", Index, Count);
			return GetData()[Index];
		}

		T& GetLast() { return At(Count - 1); }

		T& GetFirst() { return At(0); }

		T& operator[](TSizeType Index) { return At(Index); }

		TSizeType GetCount() const { return Count; }

		TSizeType GetCountInBytes() const { return Count * sizeof(T); }

		TSizeType GetCapacity() const { return	Capacity; }

		TSizeType GetCapacityInBytes() const { return Capacity * sizeof(T); }

		bool CanAdd(TSizeType AddCount) { return Count + AddCount <= GetCapacity(); }

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

		void Reserve(TSizeType NewSize)
		{
			Allocator.Allocate(sizeof(T), NewSize);
			BIT_ASSERT(Allocator.IsValid());
			Data = (T*)Allocator.GetAllocation();
			Capacity = NewSize;
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

		void Add(const T* Buffer, TSizeType BufferCount)
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

		void CheckGrow(TSizeType AddCount = 1)
		{
			if (!CanAdd(AddCount))
			{
			#ifdef BIT_SLOW_ARRAY_GROW
				Reserve((Allocator.GetAllocationSize() / sizeof(T)) + AddCount);
			#else
				Reserve((Allocator.GetAllocationSize() / sizeof(T)) * 2 + AddCount);
			#endif
			}
		}

		T* GetData() const { return Data; }

		T* GetData(TSizeType Offset) const { return Data + Offset; }

	protected:
		TAllocator Allocator;
		TSizeType Count;
		TSizeType Capacity;
		T* Data;
	};

	namespace pmr
	{
		template<
			typename T,
			typename TSizeType = DefaultContainerSizeType_t
		>
		struct TArray
		{
			typedef TArray<T, TSizeType> SelfType_t;
			typedef T ElementType_t;
			typedef TSizeType SizeType_t;

			/* Begin range for loop implementation */
			TPtrFwdIterator<T> begin() { return TPtrFwdIterator<T>(GetData()); }
			TPtrFwdIterator<T> end() { return TPtrFwdIterator<T>(GetData(Count)); }
			TConstPtrFwdIterator<T> cbegin() const { return TConstPtrFwdIterator<T>(GetData()); }
			TConstPtrFwdIterator<T> cend() const { return TConstPtrFwdIterator<T>(GetData(Count)); }
			/* End range for loop implementation */

			TArray(IAllocator& Allocator) :
				Allocator(&Allocator),
				Data(nullptr),
				Count(0),
				Capacity(0)
			{}

			TArray(TSizeType InitialCapacity, IAllocator& Allocator) :
				Allocator(&Allocator),
				Data(nullptr),
				Count(0),
				Capacity(0)
			{
				Reserve(InitialCapacity);
			}

			TArray(const SelfType_t& Copy) :
				Allocator(nullptr),
				Data(nullptr),
				Count(0),
				Capacity(0)
			{
				Allocator = Copy.Allocator;
				Reserve(Copy.GetCount());
				bit::Memcpy(Data, Copy.GetData(), Copy.GetCount());
				Count = Copy.Count;
			}
			
			TArray(SelfType_t&& Move) :
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
			
			~TArray()
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

			void Reserve(TSizeType NewSize)
			{
				Data = (T*)Allocator->Reallocate(Data, NewSize * sizeof(T), alignof(T));
				Capacity = NewSize;
				BIT_ASSERT(Data != nullptr);
			}

			BIT_FORCEINLINE T& At(TSizeType Index)
			{
				BIT_ASSERT_MSG(Count > 0 && Index < Count, "Index out of bounds. Index = %d. Count = %d\n", Index, Count);
				return GetData()[Index];
			}

			T& GetLast() { return At(Count - 1); }

			T& GetFirst() { return At(0); }

			T& operator[](TSizeType Index) { return At(Index); }

			TSizeType GetCount() const { return Count; }

			TSizeType GetCountInBytes() const { return Count * sizeof(T); }

			TSizeType GetCapacity() const { return Capacity; }

			TSizeType GetCapacityInBytes() const { return Capacity * sizeof(T); }

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

			void Add(const T* Buffer, TSizeType BufferCount)
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

			T* GetData() const { return Data; }

			T* GetData(TSizeType Offset) const { return Data + Offset; }

			bool CanAdd(TSizeType AddCount) { return Count + AddCount <= GetCapacity(); }

			IAllocator& GetAllocator() { return *Allocator; }

		protected:
			bit::IAllocator* Allocator;
			T* Data;
			TSizeType Count;
			TSizeType Capacity;
		};
	}
}
