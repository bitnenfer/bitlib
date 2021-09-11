#include <bit/types.h>
#include <bit/atomics.h>
#include <bit/memory.h>
#include <bit/allocator.h>
#include <bit/reference_counter.h>

namespace bit
{
	template<typename T>
	struct TDefaultDeleter
	{
		void operator()(T* const Ptr)
		{
			bit::Delete(Ptr);
		}
	};

	template<typename T>
	struct TAllocatorDeleter
	{
		TAllocatorDeleter() :
			Allocator(nullptr)
		{}

		TAllocatorDeleter(bit::IAllocator& Allocator) :
			Allocator(&Allocator)
		{}

		void operator()(T* const Ptr)
		{
			if (Allocator != nullptr)
			{
				Allocator->Delete(Ptr);
			}
		}

		bit::IAllocator* Allocator;
	};

	struct CNonAtomicAssignPtrPolicy
	{
		template<typename T>
		static T* Assign(T** Target, T* Source)
		{
			T* Prev = *Target;
			*Target = Source;
			return Prev;
		}
	};

	template<typename TAtomicValue>
	struct TAtomicAssignPtrPolicy
	{
		template<typename T>
		static T* Assign(T** Target, T* Source)
		{
			return (T*)bit::AtomicExchange((TAtomicValue*)Target, (TAtomicValue)Source);
		}
	};

	template<typename T, typename TRefCounter>
	struct TControlBlockBase
	{
		TControlBlockBase(T* Ptr) :
			Ptr(Ptr),
			StrongRefCounter(1),
			WeakRefCounter(0)
		{}

		void IncStrongRef() { StrongRefCounter.Increment(); }
		void IncWeakRef() { WeakRefCounter.Increment(); }
		bool DecStrongRef() { return StrongRefCounter.Decrement(); }
		bool DecWeakRef() { return WeakRefCounter.Decrement(); }
		typename TRefCounter::CounterType_t GetStrongRefCount() const { return StrongRefCounter.GetCount(); }
		typename TRefCounter::CounterType_t GetWeakRefCount() const { return WeakRefCounter.GetCount(); }

		virtual void DeletePtr() = 0;

		void ResetAll()
		{
			StrongRefCounter.Reset();
			WeakRefCounter.Reset();
		}

		TRefCounter StrongRefCounter;
		TRefCounter WeakRefCounter;
		T* Ptr;
	};
	
	template<
		typename T, 
		typename TRefCounter,
		typename TCustomDeleter
	>
	struct TControlBlock : public TControlBlockBase<T, TRefCounter>
	{
		using Base = TControlBlockBase<T, TRefCounter>;

		TControlBlock(T* Ptr, const TCustomDeleter& CustomDeleter) :
			Base(Ptr),
			CustomDeleter(CustomDeleter)
		{}

		void DeletePtr() override { if (Base::Ptr != nullptr) CustomDeleter(Base::Ptr); }

		TCustomDeleter CustomDeleter;
	};

	using TAssignPtr = TAtomicAssignPtrPolicy<int64_t>;
	using TRefCounter = TAtomicRefCounter<int64_t>;

	template<
		typename T, 
		typename TDeleter = TDefaultDeleter<T>
	>
	struct TUniquePtr : public CNonCopyable
	{
		using SelfType = TUniquePtr<T, TDeleter>;

		TUniquePtr() :
			Ptr(nullptr)
		{}

		TUniquePtr(T* InPtr) :
			Ptr(InPtr)
		{}

		TUniquePtr(T* InPtr, const TDeleter& Deleter) :
			Ptr(InPtr),
			Deleter(Deleter)
		{}

		TUniquePtr(SelfType&& Move)
		{
			TAssignPtr::Assign<T>(&Ptr, Move.Release());
			Deleter = bit::Move(Move.Deleter);
		}
		SelfType& operator=(SelfType&& Move)
		{
			Reset();
			TAssignPtr::Assign<T>(&Ptr, Move.Release());
			Deleter = bit::Move(Move.Deleter);
			return *this;
		}
		~TUniquePtr()
		{
			Reset();
		}

		T* Release()
		{
			return (T*)TAssignPtr::Assign<T>(&Ptr, nullptr);
		}
		
		void Reset(T* NewPtr)
		{
			T* OldPtr = (T*)TAssignPtr::Assign<T>(&Ptr, NewPtr);
			if (OldPtr != nullptr) Deleter(OldPtr);
		}

		void Reset(T* NewPtr, const TDeleter& InDeleter)
		{
			T* OldPtr = (T*)TAssignPtr::Assign<T>(&Ptr, NewPtr);
			if (OldPtr != nullptr) Deleter(OldPtr);
			Deleter = InDeleter;
		}

		void Reset()
		{
			T* OldPtr = (T*)TAssignPtr::Assign<T>(&Ptr, nullptr);
			if (OldPtr != nullptr) Deleter(OldPtr);;
		}

		void Swap(SelfType& Other)
		{
			TAssignPtr::Assign<T>(&Other.Ptr, TAssignPtr::Assign<T>(&Ptr, Other.Ptr));
			TDeleter OldDeleter = Deleter;
			Deleter = Other.Deleter;
			Other.Deleter = OldDeleter;
		}

		bool IsValid() const { return Ptr != nullptr; }
		T* operator->() { return Ptr; }
		T& operator*() { return *Ptr; }
		T* Get() { return Ptr; }

	private:
		TUniquePtr(const SelfType&) = delete;
		TUniquePtr operator=(const SelfType&) = delete;

		template<typename T>
		friend struct TSharedPtr;

		TDeleter Deleter;
		T* Ptr;
	};

	template<typename T>
	struct TWeakPtr;
	
	template<typename T>
	struct TSharedPtr
	{
		typedef TControlBlockBase<T, TRefCounter> ControlBlockBaseType_t;

		TSharedPtr() :
			ControlBlock(nullptr)
		{}

		TSharedPtr(T* InPtr) :
			ControlBlock(Construct(InPtr))
		{}

		template<typename TDeleter>
		TSharedPtr(T* InPtr, const TDeleter& Deleter) :
			ControlBlock(Construct(InPtr, Deleter))
		{}

		~TSharedPtr()
		{
			if (IsValid() && ControlBlock->DecStrongRef())
			{
				ControlBlock->DeletePtr();
				if (ControlBlock->GetWeakRefCount() == 0)
				{
					bit::Delete(ControlBlock);
				}
			}
		}

		TSharedPtr(const TSharedPtr<T>& Copy) 
		{
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, Copy.ControlBlock);
			if (IsValid()) ControlBlock->IncStrongRef();
		}

		TSharedPtr<T>& operator=(const TSharedPtr<T>& Copy)
		{
			Reset();
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, Copy.ControlBlock);
			if (IsValid()) ControlBlock->IncStrongRef();
			return *this;
		}

		TSharedPtr<T>& operator=(TSharedPtr<T>&& Move)
		{
			Reset();
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, Move.ControlBlock);
			TAssignPtr::Assign<ControlBlockBaseType_t>(&Move.ControlBlock, nullptr);
			return *this;
		}

		template<typename TDeleter>
		TSharedPtr<T>& operator=(TUniquePtr<T, TDeleter>&& Move)
		{
			Reset(Move.Release(), Move.Deleter);
			return *this;
		}
				
		void Reset(T* NewPtr)
		{
			if (IsValid()) DestroyIfPossible();

			if (NewPtr != nullptr)
			{
				ControlBlock = Construct(NewPtr);
			}
		}

		template<typename TDeleter>
		void Reset(T* NewPtr, const TDeleter& Deleter)
		{
			if (IsValid()) DestroyIfPossible();

			if (NewPtr != nullptr)
			{
				ControlBlock = Construct(NewPtr, Deleter);
			}
		}

		void Reset()
		{
			if (IsValid()) DestroyIfPossible();
		}

		BIT_FORCEINLINE int64_t GetUseCount() const { return ControlBlock ? ControlBlock->GetStrongRefCount() : 0; }
		BIT_FORCEINLINE int64_t GetWeakCount() const { return ControlBlock ? ControlBlock->GetWeakRefCount() : 0; }
		BIT_FORCEINLINE bool IsUnique() const { return GetUseCount() == 1; }
		BIT_FORCEINLINE bool IsValid() const { return ControlBlock != nullptr; }
		BIT_FORCEINLINE bool IsAlive() const { return ControlBlock != nullptr && ControlBlock->Ptr != nullptr; }
		BIT_FORCEINLINE T* operator->() { IsValid() ? ControlBlock->Ptr : nullptr; }
		BIT_FORCEINLINE T& operator*() { return IsValid() ? *(ControlBlock->Ptr) : nullptr; }
		BIT_FORCEINLINE T* Get() { return IsValid() ? ControlBlock->Ptr : nullptr; }

	private:
		template<typename T>
		friend struct TWeakPtr;

		TSharedPtr(ControlBlockBaseType_t* OtherControlBlock)
		{
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, OtherControlBlock);
			if (IsValid()) ControlBlock->IncStrongRef();
		}

		TSharedPtr<T>& operator=(ControlBlockBaseType_t* OtherControlBlock)
		{
			Reset();
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, OtherControlBlock);
			if (IsValid()) ControlBlock->IncStrongRef();
			return this;
		}

		bool DestroyIfPossible()
		{
			ControlBlockBaseType_t* TempControlBlock = ControlBlock;
			ControlBlock = nullptr;
			if (TempControlBlock->DecStrongRef())
			{
				TempControlBlock->DeletePtr();
				if (TempControlBlock->GetWeakRefCount() == 0)
				{
					bit::Delete(TempControlBlock);
					return true;
				}
			}
			return false;
		}

		ControlBlockBaseType_t* Construct(T* Ptr) const { return bit::New<TControlBlock<T, TRefCounter, TDefaultDeleter<T>>>(Ptr, TDefaultDeleter<T>()); }

		template<typename TDeleter>
		ControlBlockBaseType_t* Construct(T* Ptr, const TDeleter& Deleter) const { return bit::New<TControlBlock<T, TRefCounter, TDeleter>>(Ptr, Deleter); }

		ControlBlockBaseType_t* ControlBlock;
	};

	template<typename T>
	struct TWeakPtr
	{
		typedef TControlBlockBase<T, TRefCounter> ControlBlockBaseType_t;

		TWeakPtr() :
			ControlBlock(nullptr)
		{}

		TWeakPtr(const TSharedPtr<T>& SharedPtr)
		{
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, SharedPtr.ControlBlock);
			if (ControlBlock != nullptr) ControlBlock->IncWeakRef();
		}

		TWeakPtr(const TWeakPtr<T>& WeakPtr)
		{
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, WeakPtr.ControlBlock);
			if (ControlBlock != nullptr) ControlBlock->IncWeakRef();
		}

		TWeakPtr(TWeakPtr<T>&& WeakPtr)
		{
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, WeakPtr.ControlBlock);
			TAssignPtr::Assign<ControlBlockBaseType_t>(&WeakPtr.ControlBlock, nullptr);
		}

		~TWeakPtr()
		{
			Reset();
		}

		TWeakPtr<T>& operator=(const TSharedPtr<T>& SharedPtr)
		{
			Reset();
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, SharedPtr.ControlBlock);
			if (ControlBlock != nullptr) ControlBlock->IncWeakRef();
			return *this;
		}

		TWeakPtr<T>& operator=(const TWeakPtr<T>& WeakPtr)
		{
			Reset();
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, WeakPtr.ControlBlock);
			if (ControlBlock != nullptr) ControlBlock->IncWeakRef();
			return *this;
		}

		TWeakPtr<T>&& operator=(TWeakPtr<T>&& WeakPtr)
		{
			Reset();
			TAssignPtr::Assign<ControlBlockBaseType_t>(&ControlBlock, WeakPtr.ControlBlock);
			TAssignPtr::Assign<ControlBlockBaseType_t>(&WeakPtr.ControlBlock, nullptr);
			return *this;
		}

		void Reset()
		{
			if (ControlBlock != nullptr && 
				ControlBlock->DecWeakRef() &&
				ControlBlock->GetStrongRefCount() <= 0)
			{
				bit::Delete(ControlBlock);
				ControlBlock = nullptr;
			}
		}

		TSharedPtr<T> Lock()
		{
			if (!HasExpired())
				return TSharedPtr<T>(ControlBlock);
			return TSharedPtr<T>();
		}

		BIT_FORCEINLINE int64_t GetUseCount() const { return ControlBlock ? ControlBlock->GetStrongRefCount() : 0; }
		BIT_FORCEINLINE bool HasExpired() const { return GetUseCount() <= 0; }
		
	private:
		ControlBlockBaseType_t* ControlBlock;
	};

	template<typename T, typename... TArgs>
	TUniquePtr<T> MakeUnique(TArgs&& ... ConstructorArgs)
	{
		return bit::TUniquePtr<T>(bit::New<T>(bit::Forward<TArgs>(ConstructorArgs)...), TDefaultDeleter<T>());
	}

	template<typename T, typename TDeleter, typename... TArgs>
	TUniquePtr<T, TDeleter> MakeUnique(const TDeleter& Deleter, TArgs&& ... ConstructorArgs)
	{
		return bit::TUniquePtr<T, TDeleter>(bit::New<T>(bit::Forward<TArgs>(ConstructorArgs)...), Deleter);
	}

	template<typename T, typename... TArgs>
	TUniquePtr<T, TAllocatorDeleter<T>> MakeUniqueWithAllocator(bit::IAllocator& Allocator, TArgs&& ... ConstructorArgs)
	{
		return bit::TUniquePtr<T, TAllocatorDeleter<T>>(Allocator.New<T>(bit::Forward<TArgs>(ConstructorArgs)...), TAllocatorDeleter<T>(Allocator));
	}

	template<typename T, typename TDeleter, typename... TArgs>
	TUniquePtr<T, TDeleter> MakeUniqueWithAllocator(const TDeleter& Deleter, bit::IAllocator& Allocator, TArgs&& ... ConstructorArgs)
	{
		return bit::TUniquePtr<T, TDeleter>(Allocator.New<T>(bit::Forward<TArgs>(ConstructorArgs)...), Deleter);
	}

	template<typename T, typename... TArgs>
	TSharedPtr<T> MakeShared(TArgs&& ...Args)
	{
		return TSharedPtr<T>(bit::New<T>(bit::Forward<TArgs>(Args)...));
	}

	template<typename T, typename... TArgs>
	TSharedPtr<T> MakeSharedWithAllocator(bit::IAllocator& Allocator, TArgs&& ...Args)
	{
		return TSharedPtr<T>(Allocator.New<T>(bit::Forward<TArgs>(Args)...), TAllocatorDeleter<T>(Allocator));
	}
}
