#pragma once

#include <bit/core/types.h>
#include <bit/core/os/os.h>
#include <bit/container/container_allocators.h>

namespace bit
{
	template<typename T>
	struct IntrusiveLinkedList
	{
		typedef IntrusiveLinkedList<T> SelfType_t;

		IntrusiveLinkedList(T& Owner) :
			Prev(this),
			Next(this),
			Head(this),
			Owner(Owner),
			Count(0)
		{}

		~IntrusiveLinkedList()
		{
			Unlink();
		}

		void Unlink()
		{
			if (Head == this && Next != this)
			{
				Head->Next->Count = Count - 1;
				SetHeadForAll(Head->Next);
				Count = 0;
			}
			else
			{
				Head->Count -= 1;
			}
			Prev->Next = Next;
			Next->Prev = Prev;
			Next = this;
			Prev = this;
			Head = this;
		}

		void Clear()
		{
			if (Head != this)
			{
				while (Next != this)
				{
					Next->Unlink();
				}
			}
			else
			{
				Unlink();
			}
		}

		void InsertBefore(SelfType_t& Target)
		{
			Unlink();
			Next = &Target;
			Prev = Target.Prev;
			Target.Prev = this;
			Prev->Next = this;
			Head = Target.Head;
			Head->Count += 1;
		}

		void InsertAfter(SelfType_t& Target)
		{
			Unlink();
			Prev = &Target;
			Next = Target.Next;
			Target.Next = this;
			Next->Prev = this;
			Head = Target.Head;
			Head->Count += 1;
		}

		void InsertAtTail(SelfType_t& Target)
		{
			InsertBefore(*Target.Head);
		}

		void InsertAtHead(SelfType_t& Target)
		{
			InsertAfter(*Target.Head);
		}

		SelfType_t* GetFirst() const { return Head->Next; }
		SelfType_t* GetLast() const { return Head->Prev; }
		SelfType_t* GetPrev() const { return Prev; }
		SelfType_t* GetNext() const { return Next; }
		bool IsEmpty() { return GetCount() == 0; }

		T& GetOwner() const { return Owner; }
		SizeType_t GetCount()
		{
			SizeType_t Count = 0;
			for (SelfType_t* Link = Head->Next; Link != Head; Link = Link->Next)
			{
				Count += 1;
			}
			if (Count == 0 && Head == this && Next == this)
			{
				return 1;
			}
			return Count;
		}

		T& operator[](SizeType_t Index)
		{
			BIT_ASSERT_MSG(Index < Head->Count, "Linked list index out of bounds");
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

		/* Begin range for loop implementation */
		struct Iterator
		{
			Iterator(SelfType_t* Link) :
				Link(Link)
			{}

			T& operator*()
			{
				return Link->GetOwner();
			}
			T* operator->()
			{
				return &Link->GetOwner();
			}
			Iterator& operator++()
			{
				Link = Link->Next;
				return *this;
			}
			Iterator operator++(int32_t) { Iterator Self = *this; ++(*this); return Self; }

			friend bool operator==(const Iterator& A, const Iterator& B)
			{
				return A.Link == B.Link;
			}

			friend bool operator!=(const Iterator& A, const Iterator& B)
			{
				return A.Link != B.Link;
			}

			SelfType_t* Link;
		};
		struct ConstIterator
		{
			ConstIterator(const SelfType_t* Link) :
				Link(Link)
			{}

			const T& operator*()
			{
				return Link->GetOwner();
			}
			const T* operator->()
			{
				return &Link->GetOwner();
			}
			ConstIterator& operator++()
			{
				Link = Link->Next;
				return *this;
			}
			ConstIterator operator++(int32_t) { ConstIterator Self = *this; ++(*this); return Self; }

			friend bool operator==(const ConstIterator& A, const ConstIterator& B)
			{
				return A.Link == B.Link;
			}

			friend bool operator!=(const ConstIterator& A, const ConstIterator& B)
			{
				return A.Link != B.Link;
			}

			const SelfType_t* Link;
		};
		Iterator begin() { return Iterator(Head->Next); }
		Iterator end() { return Iterator(Head); }
		ConstIterator cbegin() { return ConstIterator(Head->Next); }
		ConstIterator cend() { return ConstIterator(Head); }
		/* End range for loop implementation */

	private:
		void SetHeadForAll(SelfType_t* NewHead)
		{
			for (SelfType_t* Link = Head->Next; Link != Head; Link = Link->Next)
			{
				Link->Head = NewHead;
			}
		}

		SelfType_t* Prev;
		SelfType_t* Next;
		SelfType_t* Head;
		T& Owner;
		SizeType_t Count;
	};
}
