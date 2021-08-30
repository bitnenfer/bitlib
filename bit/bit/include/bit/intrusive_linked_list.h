#pragma once

#include <bit/types.h>
#include <bit/os.h>

namespace bit
{
	template<typename T, typename TSizeType = int64_t>
	struct TIntrusiveLinkedList
	{
		typedef TIntrusiveLinkedList<T, TSizeType> SelfType_t;

		TIntrusiveLinkedList(T& Owner) :
			Prev(this),
			Next(this),
			Head(this),
			Owner(Owner),
			Count(0)
		{}

		~TIntrusiveLinkedList()
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
		TSizeType GetCount()
		{
			TSizeType Count = 0;
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

		T& operator[](TSizeType Index)
		{
			BIT_ASSERT_MSG(Index < Head->Count, "Linked list index out of bounds");
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

		/* Begin range for loop implementation */
		struct CIterator
		{
			CIterator(SelfType_t* Link) :
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
			CIterator& operator++()
			{
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

			SelfType_t* Link;
		};
		struct CConstIterator
		{
			CConstIterator(const SelfType_t* Link) :
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
			CConstIterator& operator++()
			{
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

			const SelfType_t* Link;
		};
		CIterator begin() { return CIterator(Head->Next); }
		CIterator end() { return CIterator(Head); }
		CConstIterator cbegin() { return CConstIterator(Head->Next); }
		CConstIterator cend() { return CConstIterator(Head); }
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
		TSizeType Count;
	};
}
