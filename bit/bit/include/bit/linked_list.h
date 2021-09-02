#pragma once

#include <bit/types.h>
#include <bit/allocator.h>
#include <bit/container_allocator.h>
#include <bit/os.h>

namespace bit
{
	template<typename T>
	struct TLink
	{
		T Element;
		TLink* Prev;
		TLink* Next;
	};

	template<
		typename T, 
		typename TSizeType = int64_t,
		typename TAllocator = CDefaultAllocator
	>
	struct TLinkedList
	{
		typedef TLinkedList<T, TSizeType, TAllocator> SelfType_t;
		typedef TLink<T> LinkType_t;
		typedef T ElementType_t;

	public:
		/* Begin range for loop implementation */
		struct CIterator
		{
			CIterator(LinkType_t* Link) :
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

			LinkType_t* Link;
		};

		struct CConstIterator
		{
			CConstIterator(const LinkType_t* Link) :
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

			const LinkType_t* Link;
		};

		CIterator begin() { return CIterator(Head); }
		CIterator end() { return CIterator(nullptr); }
		CConstIterator cbegin() { return CConstIterator(Head); }
		CConstIterator cend() { return CConstIterator(nullptr); }
		/* End range for loop implementation */

		TLinkedList() :
			Head(nullptr),
			Tail(nullptr),
			Count(0)
		{}

		TLinkedList(const SelfType_t& Other) :
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
			Head(Other.Head),
			Tail(Other.Tail),
			Count(Other.Count)
		{
			Other.Head = nullptr;
			Other.Tail = nullptr;
			Other.Count = 0;
		}

		~TLinkedList()
		{
			LinkType_t* Link = Head;
			while (Link != nullptr)
			{
				LinkType_t* Next = Link->Next;
				bit::Destroy(Link);
				Allocator.Free(Link);
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
				Allocator.Free(Link);
				Count -= 1;
				return true;
			}
			return false;
		}

		template<typename... TArgs>
		T* Alloc(TArgs&& ... ConstructorArgs)
		{
			LinkType_t* Link = bit::Construct(Allocator.Allocate<LinkType_t>(nullptr, 1));
			Link->Prev = nullptr;
			Link->Next = nullptr;
			bit::Construct(&Link->Element, ConstructorArgs...);
			return &InsertLink(Link)->Element;
		}

		bool Contains(const T& Element)
		{
			return FindLink(Element) != nullptr;
		}

		TSizeType GetCount() const { return Count; }
		bool IsEmpty() const { return Count == 0; }

		T& operator[](TSizeType Index)
		{
			BIT_ASSERT_MSG(Index < Count, "Linked list index out of bounds");
			TSizeType ElemIndex = 0;
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
			LinkType_t* Link = bit::Construct(Allocator.Allocate<LinkType_t>(nullptr, 1));
			Link->Prev = nullptr;
			Link->Next = nullptr;
			Link->Element = Element;
			return Link;
		}

		LinkType_t* CreateLink(T&& Element)
		{
			LinkType_t* Link = bit::Construct(Allocator.Allocate<LinkType_t>(nullptr, 1));
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

		TAllocator Allocator;
		LinkType_t* Head;
		LinkType_t* Tail;
		TSizeType Count;
	};

	namespace pmr
	{
		template<typename T, typename TSizeType = int64_t>
		struct TLinkedList
		{
			typedef TLinkedList<T, TSizeType> SelfType_t;
			typedef TLink<T> LinkType_t;
			typedef T ElementType_t;

		public:
			/* Begin range for loop implementation */
			struct CIterator
			{
				CIterator(LinkType_t* Link) :
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

				LinkType_t* Link;
			};

			struct CConstIterator
			{
				CConstIterator(const LinkType_t* Link) :
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

				const LinkType_t* Link;
			};

			CIterator begin() { return CIterator(Head); }
			CIterator end() { return CIterator(nullptr); }
			CConstIterator cbegin() { return CConstIterator(Head); }
			CConstIterator cend() { return CConstIterator(nullptr); }
			/* End range for loop implementation */

			TLinkedList(bit::IAllocator& Allocator) :
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
			T* Alloc(TArgs&& ... ConstructorArgs)
			{
				LinkType_t* Link = Allocator->New<LinkType_t>();
				Link->Prev = nullptr;
				Link->Next = nullptr;
				bit::Construct(&Link->Element, ConstructorArgs...);
				return &InsertLink(Link)->Element;
			}

			bool Contains(const T& Element)
			{
				return FindLink(Element) != nullptr;
			}

			TSizeType GetCount() const { return Count; }
			bool IsEmpty() const { return Count == 0; }

			T& operator[](TSizeType Index)
			{
				BIT_ASSERT_MSG(Index < Count, "Linked list index out of bounds");
				TSizeType ElemIndex = 0;
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

			bit::IAllocator* Allocator;
			LinkType_t* Head;
			LinkType_t* Tail;
			TSizeType Count;
		};
	}
}
