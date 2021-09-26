#include <bit/core/types.h>
#include <bit/core/os/atomics.h>
#include <bit/core/memory.h>
#include <bit/core/memory/allocator.h>
#include <bit/utility/reference_counter.h>

namespace bit
{
	template<typename T>
	struct DefaultDeleter
	{
		void operator()(T* const Ptr)
		{
			bit::Delete(Ptr);
		}
	};

	template<typename T>
	struct AllocatorDeleter
	{
		AllocatorDeleter() :
			Allocator(nullptr)
		{}

		AllocatorDeleter(bit::IAllocator& Allocator) :
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

	struct NonAtomicAssignPtrPolicy
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
	struct AtomicAssignPtrPolicy
	{
		template<typename T>
		static T* Assign(T** Target, T* Source)
		{
			return (T*)bit::AtomicExchange((TAtomicValue*)Target, (TAtomicValue)Source);
		}
	};

	template<typename T, typename RefCounter>
	struct ControlBlockBase
	{
		ControlBlockBase(T* Ptr) :
			Ptr(Ptr),
			StrongRefCounter(1),
			WeakRefCounter(0)
		{}

		void IncStrongRef() { StrongRefCounter.Increment(); }
		void IncWeakRef() { WeakRefCounter.Increment(); }
		bool DecStrongRef() { return StrongRefCounter.Decrement(); }
		bool DecWeakRef() { return WeakRefCounter.Decrement(); }
		typename RefCounter::CounterType_t GetStrongRefCount() const { return StrongRefCounter.GetCount(); }
		typename RefCounter::CounterType_t GetWeakRefCount() const { return WeakRefCounter.GetCount(); }

		virtual void DeletePtr() = 0;

		void ResetAll()
		{
			StrongRefCounter.Reset();
			WeakRefCounter.Reset();
		}

		RefCounter StrongRefCounter;
		RefCounter WeakRefCounter;
		T* Ptr;
	};
	
	template<
		typename T, 
		typename RefCounter,
		typename TCustomDeleter
	>
	struct ControlBlock : public ControlBlockBase<T, RefCounter>
	{
		using Base = ControlBlockBase<T, RefCounter>;

		ControlBlock(T* Ptr, const TCustomDeleter& CustomDeleter) :
			Base(Ptr),
			CustomDeleter(CustomDeleter)
		{}

		void DeletePtr() override { if (Base::Ptr != nullptr) CustomDeleter(Base::Ptr); }

		TCustomDeleter CustomDeleter;
	};

	using AssignPtr = AtomicAssignPtrPolicy<int64_t>;
	using RefCounter = AtomicRefCounter<int64_t>;

	template<
		typename T, 
		typename TDeleter = DefaultDeleter<T>
	>
	struct UniquePtr : public NonCopyable
	{
		using SelfType = UniquePtr<T, TDeleter>;

		UniquePtr() :
			Ptr(nullptr)
		{}

		UniquePtr(T* InPtr) :
			Ptr(InPtr)
		{}

		UniquePtr(T* InPtr, const TDeleter& Deleter) :
			Ptr(InPtr),
			Deleter(Deleter)
		{}

		UniquePtr(SelfType&& Move)
		{
			AssignPtr::Assign<T>(&Ptr, Move.Release());
			Deleter = bit::Move(Move.Deleter);
		}
		SelfType& operator=(SelfType&& Move)
		{
			Reset();
			AssignPtr::Assign<T>(&Ptr, Move.Release());
			Deleter = bit::Move(Move.Deleter);
			return *this;
		}
		~UniquePtr()
		{
			Reset();
		}

		T* Release()
		{
			return (T*)AssignPtr::Assign<T>(&Ptr, nullptr);
		}
		
		void Reset(T* NewPtr)
		{
			T* OldPtr = (T*)AssignPtr::Assign<T>(&Ptr, NewPtr);
			if (OldPtr != nullptr) Deleter(OldPtr);
		}

		void Reset(T* NewPtr, const TDeleter& InDeleter)
		{
			T* OldPtr = (T*)AssignPtr::Assign<T>(&Ptr, NewPtr);
			if (OldPtr != nullptr) Deleter(OldPtr);
			Deleter = InDeleter;
		}

		void Reset()
		{
			T* OldPtr = (T*)AssignPtr::Assign<T>(&Ptr, nullptr);
			if (OldPtr != nullptr) Deleter(OldPtr);;
		}

		void Swap(SelfType& Other)
		{
			AssignPtr::Assign<T>(&Other.Ptr, AssignPtr::Assign<T>(&Ptr, Other.Ptr));
			TDeleter OldDeleter = Deleter;
			Deleter = Other.Deleter;
			Other.Deleter = OldDeleter;
		}

		bool IsValid() const { return Ptr != nullptr; }
		T* operator->() { return Ptr; }
		T& operator*() { return *Ptr; }
		T* Get() { return Ptr; }

	private:
		UniquePtr(const SelfType&) = delete;
		UniquePtr operator=(const SelfType&) = delete;

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
		typedef ControlBlockBase<T, RefCounter> ControlBlockBaseType_t;

		TSharedPtr() :
			CtrlBlock(nullptr)
		{}

		TSharedPtr(T* InPtr) :
			CtrlBlock(Construct(InPtr))
		{}

		template<typename TDeleter>
		TSharedPtr(T* InPtr, const TDeleter& Deleter) :
			CtrlBlock(Construct(InPtr, Deleter))
		{}

		~TSharedPtr()
		{
			if (IsValid() && CtrlBlock->DecStrongRef())
			{
				CtrlBlock->DeletePtr();
				if (CtrlBlock->GetWeakRefCount() == 0)
				{
					bit::Delete(CtrlBlock);
				}
			}
		}

		TSharedPtr(const TSharedPtr<T>& Copy) 
		{
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, Copy.CtrlBlock);
			if (IsValid()) CtrlBlock->IncStrongRef();
		}

		TSharedPtr<T>& operator=(const TSharedPtr<T>& Copy)
		{
			Reset();
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, Copy.CtrlBlock);
			if (IsValid()) CtrlBlock->IncStrongRef();
			return *this;
		}

		TSharedPtr<T>& operator=(TSharedPtr<T>&& Move)
		{
			Reset();
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, Move.CtrlBlock);
			AssignPtr::Assign<ControlBlockBaseType_t>(&Move.CtrlBlock, nullptr);
			return *this;
		}

		template<typename TDeleter>
		TSharedPtr<T>& operator=(UniquePtr<T, TDeleter>&& Move)
		{
			Reset(Move.Release(), Move.Deleter);
			return *this;
		}
				
		void Reset(T* NewPtr)
		{
			if (IsValid()) DestroyIfPossible();

			if (NewPtr != nullptr)
			{
				CtrlBlock = Construct(NewPtr);
			}
		}

		template<typename TDeleter>
		void Reset(T* NewPtr, const TDeleter& Deleter)
		{
			if (IsValid()) DestroyIfPossible();

			if (NewPtr != nullptr)
			{
				CtrlBlock = Construct(NewPtr, Deleter);
			}
		}

		void Reset()
		{
			if (IsValid()) DestroyIfPossible();
		}

		BIT_FORCEINLINE int64_t GetUseCount() const { return CtrlBlock ? CtrlBlock->GetStrongRefCount() : 0; }
		BIT_FORCEINLINE int64_t GetWeakCount() const { return CtrlBlock ? CtrlBlock->GetWeakRefCount() : 0; }
		BIT_FORCEINLINE bool IsUnique() const { return GetUseCount() == 1; }
		BIT_FORCEINLINE bool IsValid() const { return CtrlBlock != nullptr; }
		BIT_FORCEINLINE bool IsAlive() const { return CtrlBlock != nullptr && CtrlBlock->Ptr != nullptr; }
		BIT_FORCEINLINE T* operator->() { IsValid() ? CtrlBlock->Ptr : nullptr; }
		BIT_FORCEINLINE T& operator*() { return IsValid() ? *(CtrlBlock->Ptr) : nullptr; }
		BIT_FORCEINLINE T* Get() { return IsValid() ? CtrlBlock->Ptr : nullptr; }

	private:
		template<typename T>
		friend struct TWeakPtr;

		TSharedPtr(ControlBlockBaseType_t* OtherControlBlock)
		{
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, OtherControlBlock);
			if (IsValid()) CtrlBlock->IncStrongRef();
		}

		TSharedPtr<T>& operator=(ControlBlockBaseType_t* OtherControlBlock)
		{
			Reset();
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, OtherControlBlock);
			if (IsValid()) CtrlBlock->IncStrongRef();
			return this;
		}

		bool DestroyIfPossible()
		{
			ControlBlockBaseType_t* TempCtrlBlock = CtrlBlock;
			CtrlBlock = nullptr;
			if (TempCtrlBlock->DecStrongRef())
			{
				TempCtrlBlock->DeletePtr();
				if (TempCtrlBlock->GetWeakRefCount() == 0)
				{
					bit::Delete(TempCtrlBlock);
					return true;
				}
			}
			return false;
		}

		ControlBlockBaseType_t* Construct(T* Ptr) const { return bit::New<ControlBlock<T, RefCounter, DefaultDeleter<T>>>(Ptr, DefaultDeleter<T>()); }

		template<typename TDeleter>
		ControlBlockBaseType_t* Construct(T* Ptr, const TDeleter& Deleter) const { return bit::New<ControlBlock<T, RefCounter, TDeleter>>(Ptr, Deleter); }

		ControlBlockBaseType_t* CtrlBlock;
	};

	template<typename T>
	struct TWeakPtr
	{
		typedef ControlBlockBase<T, RefCounter> ControlBlockBaseType_t;

		TWeakPtr() :
			CtrlBlock(nullptr)
		{}

		TWeakPtr(const TSharedPtr<T>& SharedPtr)
		{
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, SharedPtr.CtrlBlock);
			if (CtrlBlock != nullptr) CtrlBlock->IncWeakRef();
		}

		TWeakPtr(const TWeakPtr<T>& WeakPtr)
		{
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, WeakPtr.CtrlBlock);
			if (CtrlBlock != nullptr) CtrlBlock->IncWeakRef();
		}

		TWeakPtr(TWeakPtr<T>&& WeakPtr)
		{
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, WeakPtr.CtrlBlock);
			AssignPtr::Assign<ControlBlockBaseType_t>(&WeakPtr.CtrlBlock, nullptr);
		}

		~TWeakPtr()
		{
			Reset();
		}

		TWeakPtr<T>& operator=(const TSharedPtr<T>& SharedPtr)
		{
			Reset();
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, SharedPtr.CtrlBlock);
			if (CtrlBlock != nullptr) CtrlBlock->IncWeakRef();
			return *this;
		}

		TWeakPtr<T>& operator=(const TWeakPtr<T>& WeakPtr)
		{
			Reset();
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, WeakPtr.CtrlBlock);
			if (CtrlBlock != nullptr) CtrlBlock->IncWeakRef();
			return *this;
		}

		TWeakPtr<T>&& operator=(TWeakPtr<T>&& WeakPtr)
		{
			Reset();
			AssignPtr::Assign<ControlBlockBaseType_t>(&CtrlBlock, WeakPtr.CtrlBlock);
			AssignPtr::Assign<ControlBlockBaseType_t>(&WeakPtr.CtrlBlock, nullptr);
			return *this;
		}

		void Reset()
		{
			if (CtrlBlock != nullptr &&
				CtrlBlock->DecWeakRef() &&
				CtrlBlock->GetStrongRefCount() <= 0)
			{
				bit::Delete(CtrlBlock);
				CtrlBlock = nullptr;
			}
		}

		TSharedPtr<T> Lock()
		{
			if (!HasExpired())
				return TSharedPtr<T>(CtrlBlock);
			return TSharedPtr<T>();
		}

		BIT_FORCEINLINE int64_t GetUseCount() const { return CtrlBlock ? CtrlBlock->GetStrongRefCount() : 0; }
		BIT_FORCEINLINE bool HasExpired() const { return GetUseCount() <= 0; }
		
	private:
		ControlBlockBaseType_t* CtrlBlock;
	};

	template<typename T, typename... TArgs>
	UniquePtr<T> MakeUnique(TArgs&& ... ConstructorArgs)
	{
		return bit::UniquePtr<T>(bit::New<T>(bit::Forward<TArgs>(ConstructorArgs)...), DefaultDeleter<T>());
	}

	template<typename T, typename TDeleter, typename... TArgs>
	UniquePtr<T, TDeleter> MakeUnique(const TDeleter& Deleter, TArgs&& ... ConstructorArgs)
	{
		return bit::UniquePtr<T, TDeleter>(bit::New<T>(bit::Forward<TArgs>(ConstructorArgs)...), Deleter);
	}

	template<typename T, typename... TArgs>
	UniquePtr<T, AllocatorDeleter<T>> MakeUniqueWithAllocator(bit::IAllocator& Allocator, TArgs&& ... ConstructorArgs)
	{
		return bit::UniquePtr<T, AllocatorDeleter<T>>(Allocator.New<T>(bit::Forward<TArgs>(ConstructorArgs)...), AllocatorDeleter<T>(Allocator));
	}

	template<typename T, typename TDeleter, typename... TArgs>
	UniquePtr<T, TDeleter> MakeUniqueWithAllocator(const TDeleter& Deleter, bit::IAllocator& Allocator, TArgs&& ... ConstructorArgs)
	{
		return bit::UniquePtr<T, TDeleter>(Allocator.New<T>(bit::Forward<TArgs>(ConstructorArgs)...), Deleter);
	}

	template<typename T, typename... TArgs>
	TSharedPtr<T> MakeShared(TArgs&& ...Args)
	{
		return TSharedPtr<T>(bit::New<T>(bit::Forward<TArgs>(Args)...));
	}

	template<typename T, typename... TArgs>
	TSharedPtr<T> MakeSharedWithAllocator(bit::IAllocator& Allocator, TArgs&& ...Args)
	{
		return TSharedPtr<T>(Allocator.New<T>(bit::Forward<TArgs>(Args)...), AllocatorDeleter<T>(Allocator));
	}
}
