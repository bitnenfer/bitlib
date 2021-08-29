#pragma once

#include <bit/types.h>
#include <bit/allocator.h>
#include <bit/memory.h>
#include <bit/os.h>

namespace bit
{
	template<typename T>
	struct TLinkedList
	{
		typedef int32_t SizeType_t;
		typedef TLinkedList<T> SelfType_t;

	private:
		struct CLink
		{
			T Element;
			CLink* Prev;
			CLink* Next;
		};

	public:
		TLinkedList(bit::IAllocator& Allocator = bit::GetDefaultAllocator()) :
			Allocator(&Allocator),
			Head(nullptr),
			Tail(nullptr),
			Count(0)
		{}

		TLinkedList(const SelfType_t& Other) :
			Allocator(Other.Allocator),
			Head(nullptr),
			Tail(nullptr),
			Count(0)
		{
			for (const T& Element : Other)
			{
				Insert(Element);
			}
		}

		TLinkedList(SelfType_t&& Other) :
			Allocator(Other.Allocator),
			Head(Other.Head),
			Tail(Other.Tail),
			Count(Other.Count)
		{
			Other.Allocator = nullptr;
			Other.Head = nullptr;
			Other.Tail = nullptr;
			Other.Count = 0;
		}

		~TLinkedList()
		{
			CLink* Link = Head;
			while (Link != nullptr)
			{
				CLink* Next = Link->Next;
				Allocator->Delete(Link);
				Link = Next;
			}
		}

		T& Insert(const T& Element)
		{
			return InsertLink(CreateLink(Element))->Element;
		}

		T& Insert(T&& Element)
		{
			return InsertLink(CreateLink(Element))->Element;
		}

		bool Erase(const T& Element)
		{
			CLink* Link = FindLink(Element);
			if (Link != nullptr)
			{
				Unlink(Link);
				Allocator->Delete(Link);
				Count -= 1;
				return true;
			}
			return false;
		}

		template<typename... TArgs>
		T* Alloc(TArgs&& ... ConstructorArgs)
		{
			CLink* Link = Allocator->New<CLink>();
			Link->Prev = nullptr;
			Link->Next = nullptr;
			bit::Construct(&Link->Element, 1, ConstructorArgs...);
			return &InsertLink(Link)->Element;
		}

		bool Contains(const T& Element)
		{
			return FindLink(Element) != nullptr;
		}

		SizeType_t GetCount() const { return Count; }
		bool IsEmpty() const { return Count == 0; }

		/* Begin range for loop implementation */
		struct CIterator
		{
			CIterator(CLink* Link) :
				Link(Link)
			{}

			void Invalidate() 
			{
				Link = nullptr;
			}

			T& operator*()
			{
				return Link->Element;
			}
			T* operator->()
			{
				if (Link != nullptr)
					return &Link->Element;
				return nullptr;
			}
			CIterator& operator++()
			{
				if (Link != nullptr)
					Link = Link->Next;
				return *this;
			}
			CIterator operator++(int32_t) { CIterator Self = *this; ++(*this); return Self; }
			
			friend bool operator==(const CIterator& A, const CIterator& B)
			{
				return A.Link == B.Link;
			}

			friend bool operator!=(const CIterator& A, const CIterator& B)
			{
				return A.Link != B.Link;
			}

			CLink* Link;
		};

		struct CConstIterator
		{
			CConstIterator(const CLink* Link) :
				Link(Link)
			{}

			const T& operator*()
			{
				return Link->Element;
			}
			const T* operator->()
			{
				if (Link != nullptr)
					return &Link->Element;
				return nullptr;
			}
			CConstIterator& operator++()
			{
				if (Link != nullptr)
					Link = Link->Next;
				return *this;
			}
			CConstIterator operator++(int32_t) { CConstIterator Self = *this; ++(*this); return Self; }

			friend bool operator==(const CConstIterator& A, const CConstIterator& B)
			{
				return A.Link == B.Link;
			}

			friend bool operator!=(const CConstIterator& A, const CConstIterator& B)
			{
				return A.Link != B.Link;
			}

			const CLink* Link;
		};

		CIterator begin() { return CIterator(Head); }
		CIterator end() { return CIterator(nullptr); }
		CConstIterator cbegin() { return CConstIterator(Head); }
		CConstIterator cend() { return CConstIterator(nullptr); }
		/* End range for loop implementation */

		T& operator[](SizeType_t Index)
		{
			BIT_ASSERT_MSG(Index < Count, "Linked list index out of bounds");
			SizeType_t ElemIndex = 0;
			for (T& Element : *this)
			{
				if (ElemIndex++ == Index)
				{
					return Element;
				}
			}
			T* Invalid = (T*)BIT_INVALID_ADDRESS;
			BIT_ALWAYS_PANIC("Linked list index out of bounds");
			return *Invalid;
		}

		SelfType_t operator=(const SelfType_t& Other)
		{
			for (const T& Element : Other)
			{
				Insert(Element);
			}
			return *this;
		}

		SelfType_t operator=(SelfType_t&& Other)
		{
			Allocator = Other.Allocator;
			Head = Other.Head;
			Tail = Other.Tail;
			Count = Other.Count;
			Other.Allocator = nullptr;
			Other.Head = nullptr;
			Other.Tail = nullptr;
			Other.Count = 0;
			return *this;
		}

	private:
		CLink* InsertLink(CLink* NewLink)
		{
			if (Tail != nullptr)
			{
				Link(NewLink, Tail, nullptr);
			}
			else
			{
				BIT_ASSERT(Head == nullptr && Tail == nullptr);
				Head = NewLink;
			}
			Tail = NewLink;
			Count += 1;
			return NewLink;
		}

		CLink* FindLink(const T& Element)
		{
			CLink* Link = Head;
			while (Link != nullptr)
			{
				if (Link->Element == Element)
				{
					return Link;
				}
			}
			return nullptr;
		}

		CLink* CreateLink(const T& Element)
		{
			CLink* Link = Allocator->New<CLink>();
			Link->Prev = nullptr;
			Link->Next = nullptr;
			Link->Element = Element;
			return Link;
		}

		CLink* CreateLink(T&& Element)
		{
			CLink* Link = Allocator->New<CLink>();
			Link->Prev = nullptr;
			Link->Next = nullptr;
			Link->Element = Element;
			return Link;
		}

		void Link(CLink* Link, CLink* Prev, CLink* Next)
		{
			if (Prev != nullptr) Prev->Next = Link;
			if (Next != nullptr) Next->Prev = Link;
			if (Link != nullptr)
			{
				Link->Prev = Prev;
				Link->Next = Next;
			}
		}

		void Unlink(CLink* Link)
		{
			if (Link->Prev != nullptr) Link->Prev->Next = Link->Next;
			if (Link->Next != nullptr) Link->Next->Prev = Link->Prev;
			Link->Prev = nullptr;
			Link->Next = nullptr;
		}

		bit::IAllocator* Allocator;
		CLink* Head;
		CLink* Tail;
		SizeType_t Count;
	};
}
