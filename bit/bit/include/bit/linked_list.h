#pragma once

#include <bit/types.h>
#include <bit/allocator.h>
#include <bit/container_allocators.h>
#include <bit/os.h>

namespace bit
{
	template<typename TLinkType>
	struct LinkIterator
	{
		LinkIterator(TLinkType* Link) :
			Link(Link)
		{}

		void Invalidate()
		{
			Link = nullptr;
		}

		typename TLinkType::ElementType_t& operator*()
		{
			return Link->Element;
		}
		typename  TLinkType::ElementType_t* operator->()
		{
			if (Link != nullptr)
				return &Link->Element;
			return nullptr;
		}
		LinkIterator<TLinkType>& operator++()
		{
			if (Link != nullptr)
				Link = Link->Next;
			return *this;
		}
		LinkIterator<TLinkType> operator++(int32_t) { LinkIterator<TLinkType> Self = *this; ++(*this); return Self; }

		friend bool operator==(const LinkIterator<TLinkType>& A, const LinkIterator<TLinkType>& B)
		{
			return A.Link == B.Link;
		}

		friend bool operator!=(const LinkIterator<TLinkType>& A, const LinkIterator<TLinkType>& B)
		{
			return A.Link != B.Link;
		}

		TLinkType* Link;
	};

	template<typename TLinkType>
	struct ConstLinkIterator
	{
		ConstLinkIterator(const TLinkType* Link) :
			Link(Link)
		{}

		void Invalidate()
		{
			Link = nullptr;
		}

		const typename TLinkType::ElementType_t& operator*()
		{
			return Link->Element;
		}
		const typename TLinkType::ElementType_t* operator->()
		{
			if (Link != nullptr)
				return &Link->Element;
			return nullptr;
		}
		ConstLinkIterator<TLinkType>& operator++()
		{
			if (Link != nullptr)
				Link = Link->Next;
			return *this;
		}
		ConstLinkIterator<TLinkType> operator++(int32_t) { ConstLinkIterator<TLinkType> Self = *this; ++(*this); return Self; }

		friend bool operator==(const ConstLinkIterator<TLinkType>& A, const ConstLinkIterator<TLinkType>& B)
		{
			return A.Link == B.Link;
		}

		friend bool operator!=(const ConstLinkIterator<TLinkType>& A, const ConstLinkIterator<TLinkType>& B)
		{
			return A.Link != B.Link;
		}

		const TLinkType* Link;
	};

	template<typename T>
	struct NodeLink
	{
		typedef T ElementType_t;
		ElementType_t Element;
		NodeLink* Prev;
		NodeLink* Next;
	};

	template<
		typename T, 
		typename TAllocator = DefaultLinkedListAllocator
	>
	struct LinkedList
	{
		typedef LinkedList<T, TAllocator> SelfType_t;
		typedef NodeLink<T> LinkType_t;
		typedef typename TAllocator::template LinkAllocator<LinkType_t> AllocatorType_t;
		typedef T ElementType_t;
		typedef LinkIterator<LinkType_t> IteratorType_t;
		typedef ConstLinkIterator<LinkType_t> ConstIteratorType_t;

	public:
		/* Begin range for loop implementation */
		IteratorType_t begin() { return IteratorType_t(Head); }
		IteratorType_t end() { return IteratorType_t(nullptr); }
		ConstIteratorType_t cbegin() const { return ConstIteratorType_t(Head); }
		ConstIteratorType_t cend() const { return ConstIteratorType_t(nullptr); }
		/* End range for loop implementation */

		LinkedList() :
			Head(nullptr),
			Tail(nullptr),
			Count(0)
		{}

		LinkedList(const SelfType_t& Other) :
			Head(nullptr),
			Tail(nullptr),
			Count(0)
		{
			for (const T& Element : Other)
			{
				Insert(Element);
			}
		}

		LinkedList(SelfType_t&& Other) :
			Head(Other.Head),
			Tail(Other.Tail),
			Count(Other.Count)
		{
			Other.Head = nullptr;
			Other.Tail = nullptr;
			Other.Count = 0;
		}

		~LinkedList()
		{
			LinkType_t* Link = Head;
			while (Link != nullptr)
			{
				LinkType_t* Next = Link->Next;
				bit::Destroy(Link);
				Allocator.FreeLink(Link);
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
			LinkType_t* Link = FindLink(Element);
			if (Link != nullptr)
			{
				Unlink(Link);
				if (Link == Head)
				{
					Head = nullptr;
					Tail = nullptr;
				}
				bit::Destroy(Link);
				Allocator.FreeLink(Link);
				Count -= 1;
				return true;
			}
			return false;
		}

		template<typename... TArgs>
		T* New(TArgs&& ... ConstructorArgs)
		{
			LinkType_t* Link = bit::Construct(Allocator.AllocateLink(1));
			Link->Prev = nullptr;
			Link->Next = nullptr;
			bit::Construct(&Link->Element, bit::Forward<TArgs>(ConstructorArgs)...);
			return &InsertLink(Link)->Element;
		}

		bool Contains(const T& Element)
		{
			return FindLink(Element) != nullptr;
		}

		SizeType_t GetCount() const { return Count; }
		bool IsEmpty() const { return Count == 0; }

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
		LinkType_t* InsertLink(LinkType_t* NewLink)
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

		LinkType_t* FindLink(const T& Element)
		{
			LinkType_t* Link = Head;
			while (Link != nullptr)
			{
				if (Link->Element == Element)
				{
					return Link;
				}
				Link = Link->Next;
			}
			return nullptr;
		}

		LinkType_t* CreateLink(const T& Element)
		{
			LinkType_t* Link = bit::Construct(Allocator.AllocateLink(1));
			Link->Prev = nullptr;
			Link->Next = nullptr;
			Link->Element = Element;
			return Link;
		}

		LinkType_t* CreateLink(T&& Element)
		{
			LinkType_t* Link = bit::Construct(Allocator.AllocateLink(1));
			Link->Prev = nullptr;
			Link->Next = nullptr;
			Link->Element = Element;
			return Link;
		}

		void Link(LinkType_t* Link, LinkType_t* Prev, LinkType_t* Next)
		{
			if (Prev != nullptr) Prev->Next = Link;
			if (Next != nullptr) Next->Prev = Link;
			if (Link != nullptr)
			{
				Link->Prev = Prev;
				Link->Next = Next;
			}
		}

		void Unlink(LinkType_t* Link)
		{
			if (Link->Prev != nullptr) Link->Prev->Next = Link->Next;
			if (Link->Next != nullptr) Link->Next->Prev = Link->Prev;
			Link->Prev = nullptr;
			Link->Next = nullptr;
		}

		AllocatorType_t Allocator;
		LinkType_t* Head;
		LinkType_t* Tail;
		SizeType_t Count;
	};

	namespace pmr
	{
		template<typename T>
		struct LinkedList
		{
			typedef LinkedList<T> SelfType_t;
			typedef NodeLink<T> LinkType_t;
			typedef T ElementType_t;
			typedef LinkIterator<LinkType_t> IteratorType_t;
			typedef ConstLinkIterator<LinkType_t> ConstIteratorType_t;

		public:
			/* Begin range for loop implementation */			
			IteratorType_t begin() { return IteratorType_t(Head); }
			IteratorType_t end() { return IteratorType_t(nullptr); }
			ConstIteratorType_t cbegin() const { return ConstIteratorType_t(Head); }
			ConstIteratorType_t cend() const { return ConstIteratorType_t(nullptr); }
			/* End range for loop implementation */

			LinkedList(bit::Allocator& Allocator) :
				Allocator(&Allocator),
				Head(nullptr),
				Tail(nullptr),
				Count(0)
			{}

			LinkedList(const SelfType_t& Other) :
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

			LinkedList(SelfType_t&& Other) :
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

			~LinkedList()
			{
				LinkType_t* Link = Head;
				while (Link != nullptr && Allocator)
				{
					LinkType_t* Next = Link->Next;
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
				LinkType_t* Link = FindLink(Element);
				if (Link != nullptr)
				{
					Unlink(Link);
					if (Link == Head)
					{
						Head = nullptr;
						Tail = nullptr;
					}
					Allocator->Delete(Link);
					Count -= 1;
					return true;
				}
				return false;
			}

			template<typename... TArgs>
			T* New(TArgs&& ... ConstructorArgs)
			{
				LinkType_t* Link = Allocator->New<LinkType_t>();
				Link->Prev = nullptr;
				Link->Next = nullptr;
				bit::Construct(&Link->Element, bit::Forward<TArgs>(ConstructorArgs)...);
				return &InsertLink(Link)->Element;
			}

			bool Contains(const T& Element)
			{
				return FindLink(Element) != nullptr;
			}

			SizeType_t GetCount() const { return Count; }
			bool IsEmpty() const { return Count == 0; }

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
			LinkType_t* InsertLink(LinkType_t* NewLink)
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

			LinkType_t* FindLink(const T& Element)
			{
				LinkType_t* Link = Head;
				while (Link != nullptr)
				{
					if (Link->Element == Element)
					{
						return Link;
					}
					Link = Link->Next;
				}
				return nullptr;
			}

			LinkType_t* CreateLink(const T& Element)
			{
				LinkType_t* Link = Allocator->New<LinkType_t>();
				Link->Prev = nullptr;
				Link->Next = nullptr;
				Link->Element = Element;
				return Link;
			}

			LinkType_t* CreateLink(T&& Element)
			{
				LinkType_t* Link = Allocator->New<LinkType_t>();
				Link->Prev = nullptr;
				Link->Next = nullptr;
				Link->Element = Element;
				return Link;
			}

			void Link(LinkType_t* Link, LinkType_t* Prev, LinkType_t* Next)
			{
				if (Prev != nullptr) Prev->Next = Link;
				if (Next != nullptr) Next->Prev = Link;
				if (Link != nullptr)
				{
					Link->Prev = Prev;
					Link->Next = Next;
				}
			}

			void Unlink(LinkType_t* Link)
			{
				if (Link->Prev != nullptr) Link->Prev->Next = Link->Next;
				if (Link->Next != nullptr) Link->Next->Prev = Link->Prev;
				Link->Prev = nullptr;
				Link->Next = nullptr;
			}

			bit::Allocator* Allocator;
			LinkType_t* Head;
			LinkType_t* Tail;
			SizeType_t Count;
		};
	}
}
