#pragma once

#include <bit/os.h>
#include <bit/utils.h>
#include <bit/container_allocator.h>

namespace bit
{
	template<typename T, typename TSizeType>
	struct TArrayBase
	{
		static_assert(bit::TIsSigned<TSizeType>::bValue, "Size type must be signed");
		typedef TArrayBase<T, TSizeType> SelfType_t;
		typedef T ElementType_t;

		/* Begin range for loop implementation */
		struct CIterator
		{
			CIterator(T* Ptr) : Ptr(Ptr) {}
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
			CConstIterator(const T* Ptr) : Ptr(Ptr) {}
			const T& operator*() const { return *Ptr; }
			const T* operator->() const { return Ptr; }
			CConstIterator& operator++() { Ptr++; return *this; }
			CConstIterator operator++(int32_t) { CConstIterator Self = *this; ++(*this); return Self; }
			friend bool operator==(const CConstIterator& A, const CConstIterator& B) { return A.Ptr == B.Ptr; }
			friend bool operator!=(const CConstIterator& A, const CConstIterator& B) { return A.Ptr != B.Ptr; }
		private:
			const T* Ptr;
		};

		CIterator begin() { return CIterator(Data); }
		CIterator end() { return CIterator(Data + Count); }
		CConstIterator cbegin() const { return CConstIterator(Data); }
		CConstIterator cend() const { return CConstIterator(Data + Count); }
		/* End range for loop implementation */

		TArrayBase(T* Data, TSizeType Capacity) :
			Data(Data),
			Capacity(Capacity),
			Count(0)
		{}

		BIT_FORCEINLINE T& At(TSizeType Index)
		{
			BIT_ASSERT_MSG(Count > 0 && Index < Count, "Index out of bounds. Index = %d. Count = %d\n", Index, Count);
			return Data[Index];
		}

		T& GetLast() { return At(Count - 1); }
		T& GetFirst() { return At(0); }
		T& operator[](TSizeType Index) { return At(Index); }
		T* GetData() const { return Data; }
		TSizeType GetCount() const { return Count; }
		TSizeType GetCountInBytes() const { return Count * sizeof(T); }
		TSizeType GetCapacity() const { return Capacity; }
		TSizeType GetCapacityInBytes() const { return Capacity * sizeof(T); }
		bool CanAdd(TSizeType AddCount) { return Count + AddCount <= Capacity; }
		bool IsEmpty() const { return Count == 0; }
		void Reset() { Count = 0; }
		bool IsValid() { return Data != nullptr && Capacity != 0;  }

		void InvalidateImpl()
		{
			Data = nullptr;
			Capacity = 0;
			Count = 0;
		}

		void AddImpl(const T& Element)
		{
			Data[Count++] = Element;
		}
		void AddImpl(T&& Element)
		{
			Data[Count++] = bit::Move(Element);
		}
		void AddImpl(const T* Buffer, TSizeType BufferCount)
		{
			bit::Memcpy(&Data[Count], Buffer, sizeof(T) * BufferCount);
			Count += BufferCount;
		}
		void AddImpl(const SelfType_t& Other)
		{
			AddImpl(Other.GetData(), Other.GetCount());
		}
		T& AddEmptyImpl()
		{
			T DefaultElement;
			AddImpl(DefaultElement);
			return GetLast();
		}
		template<typename... TArgs>
		T& AllocateImpl(TArgs&& ... ConstructorArgs)
		{
			T* Ptr = &Data[Count++];
			bit::Construct(Ptr, ConstructorArgs...);
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

	protected:
		T* Data;
		TSizeType Capacity;
		TSizeType Count;
	};

	template<
		typename T, 
		typename TSizeType = int32_t,
		typename TAllocator = CDefaultAllocator
	>
	struct TArray : public TArrayBase<T, TSizeType>
	{
		typedef TArray<T, TSizeType, TAllocator> SelfType_t;
		typedef TArrayBase<T, TSizeType> BaseType_t;
		typedef T ElementType_t;

#if BIT_USE_INLINE_DATA
		BIT_FORCEINLINE bool UsesInlineData() const { return this->Data == &InlineData[0]; }
#endif

		TArray() :
#if BIT_USE_INLINE_DATA
			BaseType_t(InlineData, BIT_INLINE_DATA_COUNT)
#else
			BaseType_t(nullptr, 0)
#endif
		{}
		TArray(TSizeType InitialCapacity) :
#if BIT_USE_INLINE_DATA
			BaseType_t(InlineData, BIT_INLINE_DATA_COUNT)
#else
			BaseType_t(nullptr, 0)
#endif
		{
			Reserve(InitialCapacity);
		}
		TArray(const SelfType_t& Copy) :
#if BIT_USE_INLINE_DATA
			BaseType_t(InlineData, BIT_INLINE_DATA_COUNT)
#else
			BaseType_t(nullptr, 0)
#endif
		{
			Reserve(Copy.GetCount());
			bit::Memcpy(this->Data, Copy.GetData(), Copy.GetCount());
			this->Count = Copy.Count;
		}
		TArray(SelfType_t&& Move) :
#if BIT_USE_INLINE_DATA
			BaseType_t(InlineData, BIT_INLINE_DATA_COUNT)
#else
			BaseType_t(nullptr, 0)
#endif
		{
			Allocator = Move.Allocator;
#if BIT_USE_INLINE_DATA
			if (Move.UsesInlineData())
			{
				bit::Memcpy(InlineData, Move.InlineData, BIT_INLINE_DATA_COUNT * sizeof(T));
			}
			else
#endif
			{
				this->Data = Move.GetData();
			}
			this->Count = Move.Count;
			this->Capacity = Move.Capacity;
			Move.Invalidate();
		}
		~TArray()
		{
			Destroy();
		}

		void Invalidate()
		{
			this->InvalidateImpl();
		}

		void Destroy()
		{
			if (this->IsValid())
			{
				bit::DestroyArray(this->Data, this->Count);
#if BIT_USE_INLINE_DATA
				if (!UsesInlineData())
#endif
				{
					Allocator.Free(this->Data);
				}
				Invalidate();
			}
		}

		void Reserve(TSizeType NewSize)
		{
#if BIT_USE_INLINE_DATA
			bool bUsesInline = UsesInlineData();
			if (!bUsesInline)
#endif
			{
				this->Data = Allocator.Allocate<T>(this->Data, NewSize * sizeof(T));
			}
#if BIT_USE_INLINE_DATA
			else if (bUsesInline && NewSize > BIT_INLINE_DATA_COUNT)
			{
				this->Data = Allocator.Allocate<T>(nullptr, NewSize * sizeof(T));
				bit::Memcpy(this->Data, InlineData, this->Count * sizeof(T));
			}
#endif
			this->Capacity = NewSize;
			BIT_ASSERT(this->Data != nullptr);
		}

		void Add(const T& Element)
		{
			CheckGrow();
			this->AddImpl(Element);
		}
		void Add(T&& Element)
		{
			CheckGrow();
			this->AddImpl(bit::Move(Element));
		}
		void Add(const T* Buffer, TSizeType BufferCount)
		{
			CheckGrow(BufferCount);
			this->AddImpl(Buffer, BufferCount);
		}
		void Add(const BaseType_t& Other)
		{
			CheckGrow(Other.GetCount());
			this->AddImpl(Other);
		}
		SelfType_t& operator=(const SelfType_t& Copy)
		{
			bit::DestroyArray(this->Data, this->Count);
			CheckGrow(Copy.GetCount());
			bit::Memcpy(this->Data, Copy.GetData(), Copy.GetCount());
			this->Count = Copy.Count;
			return *this;
		}
		SelfType_t& operator=(SelfType_t&& Move)
		{
			Destroy();
			Allocator = Move.Allocator;
#if BIT_USE_INLINE_DATA
			if (!UsesInlineData())
#endif
			{
				this->Data = Move.Data;
			}
#if BIT_USE_INLINE_DATA
			else
			{
				bit::Memcpy(InlineData, Move.InlineData, BIT_INLINE_DATA_COUNT * sizeof(T));
				bit::Memcpy(this->Data, InlineData, this->Count);
			}
#endif
			this->Count = Move.Count;
			this->Capacity = Move.Capacity;
			Move.Invalidate();
			return *this;
		}

		template<typename... TArgs>
		T* Allocate(TArgs&&... ConstructorArgs)
		{
			CheckGrow();
			return AllocateImpl(ConstructorArgs...);
		}

		void CheckGrow(TSizeType AddCount = 1)
		{
			if (!this->CanAdd(AddCount))
			{
			#ifdef BIT_SLOW_ARRAY_GROW
				Reserve(Capacity + AddCount);
			#else
				Reserve(this->Capacity * 2 + AddCount);
			#endif
			}
		}

		TAllocator& GetAllocator() { return Allocator; }

	protected:
#if BIT_USE_INLINE_DATA
		T InlineData[BIT_INLINE_DATA_COUNT];
#endif
		TAllocator Allocator;
	};

	namespace pmr
	{
		template<
			typename T,
			typename TSizeType = int32_t
		>
		struct TArray : public TArrayBase<T, TSizeType>
		{
			typedef TArray<T, TSizeType> SelfType_t;
			typedef TArrayBase<T, TSizeType> BaseType_t;
			typedef T ElementType_t;

#if BIT_USE_INLINE_DATA
			BIT_FORCEINLINE bool UsesInlineData() const { return this->Data == &InlineData[0]; }
#endif

			TArray(IAllocator& Allocator) :
#if BIT_USE_INLINE_DATA
				BaseType_t(InlineData, BIT_INLINE_DATA_COUNT),
#else
				BaseType_t(nullptr, 0),
#endif
				Allocator(&Allocator)
			{}
			TArray(TSizeType InitialCapacity, IAllocator& Allocator) :
#if BIT_USE_INLINE_DATA
				BaseType_t(InlineData, BIT_INLINE_DATA_COUNT),
#else
				BaseType_t(nullptr, 0),
#endif
				Allocator(&Allocator)
			{
				Reserve(InitialCapacity);
			}
			TArray(const SelfType_t& Copy) :
#if BIT_USE_INLINE_DATA
				BaseType_t(InlineData, BIT_INLINE_DATA_COUNT),
#else
				BaseType_t(nullptr, 0),
#endif
				Allocator(Copy.Allocator)
			{
				Reserve(Copy.GetCount());
				bit::Memcpy(this->Data, Copy.GetData(), Copy.GetCount());
				this->Count = Copy.Count;
			}
			TArray(SelfType_t&& Move) :
#if BIT_USE_INLINE_DATA
				BaseType_t(InlineData, BIT_INLINE_DATA_COUNT)
#else
				BaseType_t(nullptr, 0)
#endif
			{
				Allocator = Move.Allocator;
#if BIT_USE_INLINE_DATA
				if (Move.UsesInlineData())
				{
					bit::Memcpy(InlineData, Move.InlineData, BIT_INLINE_DATA_COUNT * sizeof(T));
				}
				else
#endif
				{
					this->Data = Move.GetData();
				}
				this->Count = Move.Count;
				this->Capacity = Move.Capacity;
				Move.InvalidateImpl();
			}
			~TArray()
			{
				Destroy();
			}

			void Invalidate()
			{
				this->InvalidateImpl();
				Allocator = nullptr;
			}

			void Destroy()
			{
				if (this->IsValid())
				{
					bit::DestroyArray(this->Data, this->Count);
					if (Allocator != nullptr)
					{
#if BIT_USE_INLINE_DATA
						if (!UsesInlineData())
#endif
						{
							Allocator->Free(this->Data);
						}
					}
					this->InvalidateImpl();
				}
			}

			void Reserve(TSizeType NewSize)
			{
#if BIT_USE_INLINE_DATA
				bool bUsesInline = UsesInlineData();
				if (!bUsesInline)
#endif
				{
					this->Data = (T*)Allocator->Reallocate(this->Data, NewSize * sizeof(T), sizeof(T));
				}
#if BIT_USE_INLINE_DATA
				else if (bUsesInline && NewSize > BIT_INLINE_DATA_COUNT)
				{
					this->Data = (T*)Allocator->Reallocate(nullptr, NewSize * sizeof(T), sizeof(T));
					bit::Memcpy(this->Data, InlineData, this->Count * sizeof(T));
				}
#endif
				this->Capacity = NewSize;
				BIT_ASSERT(this->Data != nullptr);
			}

			void Add(const T& Element)
			{
				CheckGrow();
				this->AddImpl(Element);
			}
			void Add(T&& Element)
			{
				CheckGrow();
				this->AddImpl(bit::Move(Element));
			}
			void Add(const T* Buffer, TSizeType BufferCount)
			{
				CheckGrow(BufferCount);
				this->AddImpl(Buffer, BufferCount);
			}
			void Add(const BaseType_t& Other)
			{
				CheckGrow(Other.GetCount());
				this->AddImpl(Other);
			}
			SelfType_t& operator=(const SelfType_t& Copy)
			{
				bit::Destroy(this->Data, this->Count);
				CheckGrow(Copy.GetCount());
				bit::Memcpy(this->Data, Copy.GetData(), Copy.GetCount());
				this->Count = Copy.Count;
				return *this;
			}
			SelfType_t& operator=(SelfType_t&& Move)
			{
				Destroy();
				Allocator = Move.Allocator;
#if BIT_USE_INLINE_DATA
				if (!UsesInlineData())
#endif
				{
					this->Data = Move.Data;
				}
#if BIT_USE_INLINE_DATA
				else
				{
					bit::Memcpy(InlineData, Move.InlineData, BIT_INLINE_DATA_COUNT * sizeof(T));
					bit::Memcpy(this->Data, InlineData, this->Count);
				}
#endif
				this->Count = Move.Count;
				this->Capacity = Move.Capacity;
				Move.Invalidate();
				return *this;
			}

			template<typename... TArgs>
			T* Allocate(TArgs&& ... ConstructorArgs)
			{
				CheckGrow();
				return AllocateImpl(ConstructorArgs...);
			}

			void CheckGrow(TSizeType AddCount = 1)
			{
				if (!this->CanAdd(AddCount))
				{
	#ifdef BIT_SLOW_ARRAY_GROW
					Reserve(Capacity + AddCount);
	#else
					Reserve(this->Capacity * 2 + AddCount);
	#endif
				}
			}

			IAllocator& GetAllocator() { return *Allocator; }

		protected:
#if BIT_USE_INLINE_DATA
			T InlineData[BIT_INLINE_DATA_COUNT];
#endif
			bit::IAllocator* Allocator;
		};
	}
}
