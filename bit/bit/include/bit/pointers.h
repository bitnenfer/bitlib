#include <bit/types.h>
#include <bit/atomics.h>
#include <bit/memory.h>
#include <bit/allocator.h>
#include <bit/reference_counter.h>

namespace bit
{
	struct CDefaultDeleter
	{
		template<typename T>
		void operator()(T* Ptr) const
		{
			bit::Delete(Ptr);
		}
	};

	struct CAllocatorDeleter
	{
		CAllocatorDeleter() :
			Allocator(nullptr)
		{}

		CAllocatorDeleter(bit::IAllocator& Allocator) :
			Allocator(&Allocator)
		{}

		template<typename T>
		void operator()(T* Ptr) const
		{
			if (Allocator != nullptr)
			{
				Allocator->Delete(Ptr);
			}
		}

		bit::IAllocator* Allocator = nullptr;
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

	template<typename T>
	struct TBasePtr
	{
		TBasePtr(T* Ptr) { Info.Ptr = Ptr; }
		bool IsValid() const { return Info.Ptr != nullptr; }
		T* operator->() { return Info.Ptr; }
		T& operator*() { return *Info.Ptr; }
		T* Get() { return Info.Ptr; }

	protected:
		union
		{
			T* Ptr;
			int64_t NumValue;
		} Info;
	};

	template<typename T, typename TDeleter>
	struct TDeletablePtr : public TBasePtr<T>
	{
		using Base = TBasePtr<T>;
		TDeletablePtr() :
			Base(nullptr)
		{}

		TDeletablePtr(const TDeleter& Deleter) :
			Base(nullptr),
			Deleter(Deleter)
		{
			Base::Info.Ptr = nullptr;
		}

		bool IsValid() const { return Base::Info.Ptr != nullptr; }
		T* operator->() { return Base::Info.Ptr; }
		T& operator*() { return *Base::Info.Ptr; }
		T* Get() { return Base::Info.Ptr; }

	protected:
		TDeleter Deleter;
	};

	template<
		typename T, 
		typename TDeleter = CDefaultDeleter, 
		typename TAssignPtrPolicy = TAtomicAssignPtrPolicy<int64_t>
	>
	struct TUniquePtr : public TDeletablePtr<T, TDeleter>
	{
		using Base = TDeletablePtr<T, TDeleter>;
		using SelfType = TUniquePtr<T, TDeleter, TAssignPtrPolicy>;

		TUniquePtr() :
			Base()
		{}

		TUniquePtr(T* Ptr) :
			Base()
		{
			Reset(Ptr);
		}

		TUniquePtr(T* Ptr, const TDeleter& Deleter) :
			Base(Deleter)
		{
			Reset(Ptr);
		}

		TUniquePtr(SelfType&& Move) :
			Base()
		{
			Reset();
			typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, Move.Release());
			Base::Deleter = bit::Move(Move.Deleter);
		}
		SelfType& operator=(SelfType&& Move)
		{
			Reset();
			typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, Move.Release());
			Base::Deleter = bit::Move(Move.Deleter);
			return *this;
		}
		~TUniquePtr()
		{
			Reset();
		}

		T* Release()
		{
			return (T*)typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, nullptr);
		}
		
		void Reset(T* NewPtr)
		{
			T* OldPtr = (T*)typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, NewPtr);
			if (OldPtr != nullptr) this->Deleter(OldPtr);
		}

		void Reset(T* NewPtr, const TDeleter& InDeleter)
		{
			T* OldPtr = (T*)typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, NewPtr);
			if (OldPtr != nullptr) Base::Deleter(OldPtr);
			Base::Deleter = InDeleter;
		}

		void Reset()
		{
			T* OldPtr = (T*)typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, nullptr);
			if (OldPtr != nullptr) Base::Deleter(OldPtr);;
		}

		void Swap(SelfType& Other)
		{
			typename TAssignPtrPolicy::template Assign<T>(&Other.Info.Ptr, typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, Other.Info.Ptr));
			TDeleter OldDeleter = Base::Deleter;
			Base::Deleter = Other.Deleter;
			Other.Deleter = OldDeleter;
		}

	private:
		TUniquePtr(const SelfType&) = delete;
		TUniquePtr operator=(const SelfType&) = delete;
	};

	template<
		typename T, 
		typename TDeleter = CDefaultDeleter,
		typename TRefCounter = TAtomicRefCounter<int64_t>,
		typename TAssignPtrPolicy = TAtomicAssignPtrPolicy<int64_t>
	>
	struct TSharedPtr : public TDeletablePtr<T, TDeleter>
	{
		using Base = TDeletablePtr<T, TDeleter>;

		TSharedPtr() :
			Base(),
			RefCounter(nullptr)
		{}

		TSharedPtr(const TDeleter& Deleter) :
			Base(Deleter),
			RefCounter(nullptr)
		{}

		~TSharedPtr()
		{
			if (RefCounter != nullptr && RefCounter->Decrement())
			{
				Base::Deleter(Base::Info.Ptr);
				Base::Deleter(RefCounter);
			}
		}

		TSharedPtr(const TSharedPtr<T>& Copy) :
			Base()
		{
			typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, Copy.Info.Ptr);
			typename TAssignPtrPolicy::template Assign<TRefCounter>(&RefCounter, Copy.RefCounter);
			if (RefCounter != nullptr) RefCounter->Increment();
		}

		TSharedPtr<T>& operator=(const TSharedPtr<T>& Copy)
		{
			typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, Copy.Info.Ptr);
			typename TAssignPtrPolicy::template Assign<TRefCounter>(&RefCounter, Copy.RefCounter);
			if (RefCounter != nullptr) RefCounter->Increment();
			return *this;
		}
		
		void Reset(T* NewPtr)
		{
			T* OldPtr = (T*)typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, NewPtr);
			if (OldPtr != nullptr) Base::Deleter(OldPtr);
			if (RefCounter != nullptr) RefCounter->Reset();
		}

		void Reset()
		{
			T* OldPtr = (T*)typename TAssignPtrPolicy::template Assign<T>(&Base::Info.Ptr, nullptr);
			if (OldPtr != nullptr) Base::Deleter(OldPtr);
			if (RefCounter != nullptr) RefCounter->Reset();
		}

		int64_t GetRefCount() const { return RefCounter ? RefCounter->GetCount() : 0; }
		bool IsUnique() const { return RefCounter ? RefCounter->GetCount() == 1 : true; }

	private:
		template<typename T, typename... TArgs>
		friend TSharedPtr<T> MakeShared(TArgs&& ...Args);

		template<typename... TArgs>
		T* Create(TArgs&& ...Args)
		{
			RefCounter = bit::New<TAtomicRefCounter<int64_t>>(1);
			Base::Info.Ptr = bit::New<T>(bit::Forward<TArgs>(Args)...);
			return Base::Info.Ptr;
		}

		TRefCounter* RefCounter;
	};

	template<typename T, typename... TArgs>
	TUniquePtr<T> MakeUnique(TArgs&& ... ConstructorArgs)
	{
		return bit::TUniquePtr<T>(bit::New<T>(bit::Forward<TArgs>(ConstructorArgs)...));
	}

	template<typename T, typename... TArgs>
	TSharedPtr<T> MakeShared(TArgs&& ...Args)
	{
		TSharedPtr<T> Ptr;
		Ptr.Create(bit::Forward<TArgs>(Args)...);
		return Ptr;
	}

	namespace pmr
	{
		template<
			typename T,
			typename TDeleter = CDefaultDeleter,
			typename TAssignPtrPolicy = TAtomicAssignPtrPolicy<int64_t>
		>
		using TUniquePtr = bit::TUniquePtr<T, bit::CAllocatorDeleter, TAssignPtrPolicy>;

		template<typename T, typename... TArgs>
		pmr::TUniquePtr<T> MakeUnique(bit::IAllocator& Allocator, TArgs&& ... ConstructorArgs)
		{
			return pmr::TUniquePtr<T>(Allocator.New<T>(bit::Forward<TArgs>(ConstructorArgs)...), CAllocatorDeleter(Allocator));
		}
	}
}
