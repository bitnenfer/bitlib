#pragma once

#include <bit/types.h>
#include <bit/allocator.h>
#include <bit/murmur_hash.h>
#include <bit/memory.h>
#include <bit/linked_list.h>

namespace bit
{
	template<typename TKey, typename TValue>
	struct TKeyValue
	{
		TKey Key;
		TValue Value;
	};

	template<
		typename TKey,
		typename TValue,
		typename TSizeType = int64_t,
		typename THashType = size_t,
		typename THash = TMurmurHash<TKey>,
		typename TAllocator = CDefaultAllocator
	>
		struct THashTable
	{
		typedef THashTable<TKey, TValue, TSizeType, THashType, THash, TAllocator> SelfType_t;

	private:
		struct CBucketEntry
		{
			TKeyValue<TKey, TValue> Pair;
			THashType Hash;
		};

		struct CBucket
		{
			bool Erase(THashType Hash, const TKey& Key)
			{
				CBucketEntry* Entry = Find(Hash, Key);
				if (Entry != nullptr)
				{
					return List.Erase(*Entry);
				}
				return false;
			}

			CBucketEntry* Insert(THashType Hash, const TKey& Key, const TValue& Value)
			{
				return SetOrInsert(Hash, Key, Value);
			}

			CBucketEntry* SetOrInsert(THashType Hash, const TKey& Key, const TValue& Value)
			{
				for (CBucketEntry& Entry : List)
				{
					if (Entry.Hash == Hash && Entry.Pair.Key == Key)
					{
						return Set(&Entry, Hash, Key, Value);
					}
				}
				return CreateAndInsert(Hash, Key, Value);
			}

			CBucketEntry* CreateAndInsert(THashType Hash, const TKey& Key, const TValue& Value)
			{
				return Set(List.Alloc(), Hash, Key, Value);
			}

			CBucketEntry* Set(CBucketEntry* Entry, THashType Hash, const TKey& Key, const TValue& Value)
			{
				Entry->Hash = Hash;
				Entry->Pair.Key = Key;
				Entry->Pair.Value = Value;
				return Entry;
			}

			CBucketEntry* Find(THashType Hash, const TKey& Key)
			{
				for (CBucketEntry& Entry : List)
				{
					if (Entry.Hash == Hash && Entry.Pair.Key == Key)
					{
						return &Entry;
					}
				}
				return nullptr;
			}

			TLinkedList<CBucketEntry, TSizeType, TAllocator> List;
		};

		struct CTableKey
		{
			THashType Hash;
			TSizeType BucketIndex;
		};

	public:

		/* Begin range for loop implementation */
		struct CIterator
		{
			CIterator(CBucket* Buckets, typename TLinkedList<CBucketEntry, TSizeType, TAllocator>::CIterator Iter, TSizeType BucketIndex, TSizeType BucketCount) :
				Buckets(Buckets),
				Iter(Iter),
				BucketIndex(BucketIndex),
				BucketCount(BucketCount)
			{}

			TKeyValue<TKey, TValue>& operator*()
			{
				return Iter->Pair;
			}

			TKeyValue<TKey, TValue>* operator->()
			{
				if (Iter != Buckets[BucketIndex].List.end())
					return &Iter->Pair;
				return nullptr;
			}

			CIterator& operator++()
			{
				if (++Iter == Buckets[BucketIndex].List.end())
				{
					for (TSizeType Index = BucketIndex + 1; Index < BucketCount; ++Index)
					{
						if (!Buckets[Index].List.IsEmpty())
						{
							Iter = Buckets[Index].List.begin();
							BucketIndex = Index;
							return *this;
						}
					}
					Iter.Invalidate();
				}
				return *this;
			}

			CIterator operator++(int32_t) { CIterator Self = *this; ++(*this); return Self; }

			friend bool operator==(const CIterator& A, const CIterator& B)
			{
				return A.Iter == B.Iter;
			}

			friend bool operator!=(const CIterator& A, const CIterator& B)
			{
				return A.Iter != B.Iter;
			}

		private:
			CBucket* Buckets;
			typename TLinkedList<CBucketEntry, TSizeType, TAllocator>::CIterator Iter;
			TSizeType BucketIndex;
			TSizeType BucketCount;
		};

		CIterator begin()
		{
			return CIterator(Buckets, Buckets[ClosestBucket].List.begin(), ClosestBucket, BucketCount);
		}
		CIterator end()
		{
			return CIterator(Buckets, Buckets[FurthestBucket].List.end(), FurthestBucket, BucketCount);
		}

		/* End range for loop implementation */

		THashTable() :
			Buckets(nullptr),
			BucketCount(0),
			ElementCount(0),
			FurthestBucket(0),
			ClosestBucket(0)
		{}

		THashTable(TSizeType InitialCapacity) :
			Buckets(nullptr),
			BucketCount(0),
			ElementCount(0),
			FurthestBucket(0),
			ClosestBucket(0)
		{
			ReHash(InitialCapacity);
		}

		~THashTable()
		{
			bit::DestroyArray(Buckets, BucketCount);
			Allocator.Free(Buckets);
			Buckets = nullptr;
			BucketCount = 0;
			ElementCount = 0;
		}

		void ReHash(TSizeType NewSize)
		{
			if (Buckets != nullptr)
			{
				FurthestBucket = 0;
				ClosestBucket = NewSize;
				CBucket* NewBuckets = Allocator.Allocate<CBucket>(nullptr, NewSize);
				bit::ConstructArray<CBucket>(NewBuckets, NewSize);
				for (TSizeType Index = 0; Index < BucketCount; ++Index)
				{
					CBucket& Bucket = Buckets[Index];
					for (CBucketEntry& Entry : Bucket.List)
					{
						TSizeType Index = Entry.Hash % NewSize;
						NewBuckets[Index].List.Insert(Entry);
						FurthestBucket = bit::Max(FurthestBucket, Index);
						ClosestBucket = bit::Min(ClosestBucket, Index);
					}
				}
				bit::DestroyArray(Buckets, BucketCount);
				Allocator.Free(Buckets);
				Buckets = NewBuckets;
				BucketCount = NewSize;
			}
			else
			{
				Buckets = bit::ConstructArray<CBucket>(Allocator.Allocate<CBucket>(nullptr, NewSize), NewSize);
				if (Buckets != nullptr) BucketCount = NewSize;
				ClosestBucket = BucketCount;
			}
		}

		TValue& Insert(const TKey& Key, const TValue& Value)
		{
			CheckGrow();
			ElementCount += 1;
			CTableKey TableKey = GetTableKey(Key);
			FurthestBucket = bit::Max(FurthestBucket, TableKey.BucketIndex);
			ClosestBucket = bit::Min(ClosestBucket, TableKey.BucketIndex);
			return Buckets[TableKey.BucketIndex].Insert(TableKey.Hash, Key, Value)->Pair.Value;
		}

		TValue& Insert(const TKey& Key, TValue&& Value)
		{
			CheckGrow();
			ElementCount += 1;
			CTableKey TableKey = GetTableKey(Key);
			FurthestBucket = bit::Max(FurthestBucket, TableKey.BucketIndex);
			ClosestBucket = bit::Min(ClosestBucket, TableKey.BucketIndex);
			return Buckets[TableKey.BucketIndex].Insert(TableKey.Hash, Key, bit::Move(Value))->Pair.Value;
		}

		bool Erase(const TKey& Key)
		{
			CTableKey TableKey = GetTableKey(Key);
			CBucket& Bucket = Buckets[TableKey.BucketIndex];
			if (Bucket.Erase(TableKey.Hash, Key))
			{
				if (Bucket.List.IsEmpty())
				{
					FurthestBucket = 0;
					ClosestBucket = BucketCount;
					for (TSizeType Index = 0; Index < TableKey.BucketIndex; ++Index)
					{
						if (!Buckets[Index].List.IsEmpty())
						{
							FurthestBucket = bit::Max(FurthestBucket, Index);
							ClosestBucket = bit::Min(ClosestBucket, Index);
						}
					}
				}
				ElementCount -= 1;
				return true;
			}
			return false;
		}

		bool Contains(const TKey& Key)
		{
			CTableKey TableKey = GetTableKey(Key);
			return Buckets[TableKey.BucketIndex].Find(TableKey.Hash, Key) != nullptr;
		}

		TValue& operator[](const TKey& Key)
		{
			CTableKey TableKey = GetTableKey(Key);
			CBucketEntry* Entry = Buckets[TableKey.BucketIndex].Find(TableKey.Hash, Key);
			if (Entry != nullptr)
			{
				return Entry->Pair.Value;
			}
			TValue Default{};
			return Insert(Key, Default);
		}

		void CheckGrow(TSizeType AddCount = 1)
		{
			if ((double)ElementCount + AddCount >= (double)BucketCount * 0.8)
			{
				ReHash((BucketCount + AddCount) * 4);
			}
		}

		THashType GetHash(const TKey& Key) const { return Hasher(Key); }
		TSizeType GetCount() const { return ElementCount; }
		TSizeType GetCapacity() const { return BucketCount; }
		bool IsEmpty() const { return ElementCount == 0; }



	private:
		CTableKey GetTableKey(const TKey& Key) const
		{
			THashType Hash = GetHash(Key);
			TSizeType Index = Hash % BucketCount;
			return { Hash, Index };
		}

		TAllocator Allocator;
		CBucket* Buckets;
		TSizeType BucketCount;
		TSizeType ElementCount;
		TSizeType FurthestBucket;
		TSizeType ClosestBucket;
		THash Hasher;
	};

	namespace pmr
	{
		template<
			typename TKey,
			typename TValue,
			typename TSizeType = int64_t,
			typename THashType = size_t,
			typename THash = TMurmurHash<TKey>
		>
		struct THashTable
		{
			typedef THashTable<TKey, TValue, TSizeType, THashType, THash> SelfType_t;

		private:
			struct CBucketEntry
			{
				TKeyValue<TKey, TValue> Pair;
				THashType Hash;
			};

			struct CBucket
			{
				CBucket(bit::IAllocator* Allocator) :
					List(*Allocator)
				{}

				bool Erase(THashType Hash, const TKey& Key)
				{
					CBucketEntry* Entry = Find(Hash, Key);
					if (Entry != nullptr)
					{
						return List.Erase(*Entry);
					}
					return false;
				}

				CBucketEntry* Insert(THashType Hash, const TKey& Key, const TValue& Value)
				{
					return SetOrInsert(Hash, Key, Value);
				}

				CBucketEntry* SetOrInsert(THashType Hash, const TKey& Key, const TValue& Value)
				{
					for (CBucketEntry& Entry : List)
					{
						if (Entry.Hash == Hash && Entry.Pair.Key == Key)
						{
							return Set(&Entry, Hash, Key, Value);
						}
					}
					return CreateAndInsert(Hash, Key, Value);
				}

				CBucketEntry* CreateAndInsert(THashType Hash, const TKey& Key, const TValue& Value)
				{
					return Set(List.Alloc(), Hash, Key, Value);
				}

				CBucketEntry* Set(CBucketEntry* Entry, THashType Hash, const TKey& Key, const TValue& Value)
				{
					Entry->Hash = Hash;
					Entry->Pair.Key = Key;
					Entry->Pair.Value = Value;
					return Entry;
				}

				CBucketEntry* Find(THashType Hash, const TKey& Key)
				{
					for (CBucketEntry& Entry : List)
					{
						if (Entry.Hash == Hash && Entry.Pair.Key == Key)
						{
							return &Entry;
						}
					}
					return nullptr;
				}

				pmr::TLinkedList<CBucketEntry, TSizeType> List;
			};

			struct CTableKey
			{
				THashType Hash;
				TSizeType BucketIndex;
			};

		public:

			/* Begin range for loop implementation */
			struct CIterator
			{
				CIterator(CBucket* Buckets, typename pmr::TLinkedList<CBucketEntry>::CIterator Iter, TSizeType BucketIndex, TSizeType BucketCount) :
					Buckets(Buckets),
					Iter(Iter),
					BucketIndex(BucketIndex),
					BucketCount(BucketCount)
				{}

				TKeyValue<TKey, TValue>& operator*()
				{
					if ((uintptr_t)&Iter->Pair == 0xFFFFFFFFFFFFFFFB)
					{
						BIT_DEBUG_BREAK();
					}
					return Iter->Pair;
				}

				TKeyValue<TKey, TValue>* operator->()
				{
					if (Iter != Buckets[BucketIndex].List.end())
						return &Iter->Pair;
					return nullptr;
				}

				CIterator& operator++()
				{
					if (++Iter == Buckets[BucketIndex].List.end())
					{
						for (TSizeType Index = BucketIndex + 1; Index < BucketCount; ++Index)
						{
							if (!Buckets[Index].List.IsEmpty())
							{
								Iter = Buckets[Index].List.begin();
								BucketIndex = Index;
								return *this;
							}
						}
						Iter.Invalidate();
					}
					return *this;
				}

				CIterator operator++(int32_t) { CIterator Self = *this; ++(*this); return Self; }

				friend bool operator==(const CIterator& A, const CIterator& B)
				{
					return A.Iter == B.Iter;
				}

				friend bool operator!=(const CIterator& A, const CIterator& B)
				{
					return A.Iter != B.Iter;
				}

			private:
				CBucket* Buckets;
				typename pmr::TLinkedList<CBucketEntry>::CIterator Iter;
				TSizeType BucketIndex;
				TSizeType BucketCount;
			};

			CIterator begin()
			{
				return CIterator(Buckets, Buckets[ClosestBucket].List.begin(), ClosestBucket, BucketCount);
			}
			CIterator end()
			{
				return CIterator(Buckets, Buckets[FurthestBucket].List.end(), FurthestBucket, BucketCount);
			}

			/* End range for loop implementation */

			THashTable(bit::IAllocator& Allocator) :
				Allocator(&Allocator),
				Buckets(nullptr),
				BucketCount(0),
				ElementCount(0),
				FurthestBucket(0),
				ClosestBucket(0)
			{}

			THashTable(TSizeType InitialCapacity, bit::IAllocator& Allocator) :
				Allocator(&Allocator),
				Buckets(nullptr),
				BucketCount(0),
				ElementCount(0),
				FurthestBucket(0),
				ClosestBucket(0)
			{
				ReHash(InitialCapacity);
			}

			~THashTable()
			{
				bit::DestroyArray(Buckets, BucketCount);
				if (Allocator != nullptr)
				{
					Allocator->Free(Buckets);
				}
				Buckets = nullptr;
				BucketCount = 0;
				ElementCount = 0;
			}

			void ReHash(TSizeType NewSize)
			{
				if (Buckets != nullptr)
				{
					FurthestBucket = 0;
					ClosestBucket = NewSize;
					CBucket* NewBuckets = Allocator->Allocate<CBucket>(NewSize);
					bit::ConstructArray<CBucket>(NewBuckets, NewSize, Allocator);
					for (TSizeType Index = 0; Index < BucketCount; ++Index)
					{
						CBucket& Bucket = Buckets[Index];
						for (CBucketEntry& Entry : Bucket.List)
						{
							TSizeType Index = Entry.Hash % NewSize;
							NewBuckets[Index].List.Insert(Entry);
							FurthestBucket = bit::Max(FurthestBucket, Index);
							ClosestBucket = bit::Min(ClosestBucket, Index);
						}
					}
					bit::DestroyArray(Buckets, BucketCount);
					Allocator->Free(Buckets);
					Buckets = NewBuckets;
					BucketCount = NewSize;
				}
				else
				{
					Buckets = bit::ConstructArray<CBucket>(Allocator->Allocate<CBucket>(NewSize), NewSize, Allocator);
					if (Buckets != nullptr) BucketCount = NewSize;
					ClosestBucket = BucketCount;
				}
			}

			TValue& Insert(const TKey& Key, const TValue& Value)
			{
				CheckGrow();
				ElementCount += 1;
				CTableKey TableKey = GetTableKey(Key);
				FurthestBucket = bit::Max(FurthestBucket, TableKey.BucketIndex);
				ClosestBucket = bit::Min(ClosestBucket, TableKey.BucketIndex);
				return Buckets[TableKey.BucketIndex].Insert(TableKey.Hash, Key, Value)->Pair.Value;
			}

			TValue& Insert(const TKey& Key, TValue&& Value)
			{
				CheckGrow();
				ElementCount += 1;
				CTableKey TableKey = GetTableKey(Key);
				FurthestBucket = bit::Max(FurthestBucket, TableKey.BucketIndex);
				ClosestBucket = bit::Min(ClosestBucket, TableKey.BucketIndex);
				return Buckets[TableKey.BucketIndex].Insert(TableKey.Hash, Key, bit::Move(Value))->Pair.Value;
			}

			bool Erase(const TKey& Key)
			{
				CTableKey TableKey = GetTableKey(Key);
				CBucket& Bucket = Buckets[TableKey.BucketIndex];
				if (Bucket.Erase(TableKey.Hash, Key))
				{
					if (Bucket.List.IsEmpty())
					{
						FurthestBucket = 0;
						ClosestBucket = BucketCount;
						for (TSizeType Index = 0; Index < TableKey.BucketIndex; ++Index)
						{
							if (!Buckets[Index].List.IsEmpty())
							{
								FurthestBucket = bit::Max(FurthestBucket, Index);
								ClosestBucket = bit::Min(ClosestBucket, Index);
							}
						}
					}
					ElementCount -= 1;
					return true;
				}
				return false;
			}

			bool Contains(const TKey& Key)
			{
				CTableKey TableKey = GetTableKey(Key);
				return Buckets[TableKey.BucketIndex].Find(TableKey.Hash, Key) != nullptr;
			}

			TValue& operator[](const TKey& Key)
			{
				CTableKey TableKey = GetTableKey(Key);
				CBucketEntry* Entry = Buckets[TableKey.BucketIndex].Find(TableKey.Hash, Key);
				if (Entry != nullptr)
				{
					return Entry->Pair.Value;
				}
				TValue Default{};
				return Insert(Key, Default);
			}

			void CheckGrow(TSizeType AddCount = 1)
			{
				if ((double)ElementCount + AddCount >= (double)BucketCount * 0.8)
				{
					ReHash((BucketCount + AddCount) * 4);
				}
			}

			THashType GetHash(const TKey& Key) const { return Hasher(Key); }
			TSizeType GetCount() const { return ElementCount; }
			TSizeType GetCapacity() const { return BucketCount; }
			bool IsEmpty() const { return ElementCount == 0; }

			

		private:
			CTableKey GetTableKey(const TKey& Key) const
			{
				THashType Hash = GetHash(Key);
				TSizeType Index = Hash % BucketCount;
				return { Hash, Index };
			}

			IAllocator* Allocator;
			CBucket* Buckets;
			TSizeType BucketCount;
			TSizeType ElementCount;
			TSizeType FurthestBucket;
			TSizeType ClosestBucket;
			THash Hasher;
		};
	}
}
