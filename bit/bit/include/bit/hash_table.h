#pragma once

#include <bit/types.h>
#include <bit/allocator.h>
#include <bit/murmur_hash.h>
#include <bit/memory.h>
#include <bit/linked_list.h>

namespace bit
{
	template<typename TKey, typename TValue >
	struct TKeyValue
	{
		TKey Key;
		TValue Value;
	};

	template<
		typename TKey, 
		typename TValue, 
		typename THash = TMurmurHash<TKey>
	>
	struct THashTable
	{
		typedef int32_t SizeType_t;
		typedef size_t HashType_t;
		typedef THashTable<TKey, TValue, THash> SelfType_t;

	private:
		struct CBucketEntry
		{
			TKeyValue<TKey, TValue> Pair;
			HashType_t Hash;
		};

		struct CBucket
		{
			CBucket(bit::IAllocator* Allocator) :
				List(*Allocator)
			{}

			bool Erase(HashType_t Hash, const TKey& Key)
			{
				CBucketEntry* Entry = Find(Hash, Key);
				if (Entry != nullptr)
				{
					return List.Erase(*Entry);
				}
				return false;
			}

			CBucketEntry* Insert(HashType_t Hash, const TKey& Key, const TValue& Value)
			{
				return SetOrInsert(Hash, Key, Value);
			}

			CBucketEntry* SetOrInsert(HashType_t Hash, const TKey& Key, const TValue& Value)
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

			CBucketEntry* CreateAndInsert(HashType_t Hash, const TKey& Key, const TValue& Value)
			{
				return Set(List.Alloc(), Hash, Key, Value);
			}

			CBucketEntry* Set(CBucketEntry* Entry, HashType_t Hash, const TKey& Key, const TValue& Value)
			{
				Entry->Hash = Hash;
				Entry->Pair.Key = Key;
				Entry->Pair.Value = Value;
				return Entry;
			}

			CBucketEntry* Find(HashType_t Hash, const TKey& Key)
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

			TLinkedList<CBucketEntry> List;
		};

		struct CTableKey
		{
			HashType_t Hash;
			SizeType_t BucketIndex;
		};

	public:
		THashTable(bit::IAllocator& Allocator = bit::GetDefaultAllocator()) :
			Allocator(&Allocator),
			Buckets(nullptr),
			BucketCount(0),
			ElementCount(0),
			FurthestBucket(0),
			ClosestBucket(0)
		{}

		THashTable(SizeType_t InitialCapacity, bit::IAllocator& Allocator = bit::GetDefaultAllocator()) :
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
			bit::Destroy(Buckets, BucketCount);
			Allocator->Free(Buckets);
			Buckets = nullptr;
			BucketCount = 0;
			ElementCount = 0;
		}

		void ReHash(SizeType_t NewSize)
		{
			if (Buckets != nullptr)
			{
				FurthestBucket = 0;
				ClosestBucket = NewSize;
				CBucket* NewBuckets = bit::Construct<CBucket>(Allocator->Alloc<CBucket>(NewSize), NewSize, Allocator);
				for (SizeType_t Index = 0; Index < BucketCount; ++Index)
				{
					CBucket& Bucket = Buckets[Index];
					for (CBucketEntry& Entry : Bucket.List)
					{
						SizeType_t Index = Entry.Hash % NewSize;
						NewBuckets[Index].List.Insert(Entry);
						FurthestBucket = bit::Max(FurthestBucket, Index);
						ClosestBucket = bit::Min(ClosestBucket, Index);
					}
				}
				bit::Destroy(Buckets, BucketCount);
				Allocator->Free(Buckets);
				Buckets = NewBuckets;
				BucketCount = NewSize;
			}
			else
			{
				Buckets = bit::Construct<CBucket>(Allocator->Alloc<CBucket>(NewSize), NewSize, Allocator);
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
				if (Bucket.IsEmpty())
				{
					FurthestBucket = 0;
					ClosestBucket = BucketCount;
					for (SizeType_t Index = 0; Index < TableKey.BucketIndex; ++Index)
					{
						if (!Buckets[Index].IsEmpty())
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

		void CheckGrow(SizeType_t AddCount = 1)
		{
			if ((double)ElementCount + AddCount >= (double)BucketCount * 0.8)
			{
				ReHash((BucketCount + AddCount) * 4);
			}
		}

		HashType_t GetHash(const TKey& Key) const { return Hasher(Key); }
		SizeType_t GetCount() const { return ElementCount; }
		SizeType_t GetCapacity() const { return BucketCount; }
		bool IsEmpty() const { return ElementCount == 0; }

		/* Begin range for loop implementation */
		struct CIterator
		{
			CIterator(CBucket* Buckets, typename TLinkedList<CBucketEntry>::CIterator Iter, SizeType_t BucketIndex, SizeType_t BucketCount) :
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
					for (SizeType_t Index = BucketIndex + 1; Index < BucketCount; ++Index)
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
			typename TLinkedList<CBucketEntry>::CIterator Iter;
			SizeType_t BucketIndex;
			SizeType_t BucketCount;
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

	private:
		CTableKey GetTableKey(const TKey& Key) const 
		{
			HashType_t Hash = GetHash(Key);
			SizeType_t Index = Hash % BucketCount;
			return { Hash, Index }; 
		}
		
		IAllocator* Allocator;
		CBucket* Buckets;
		SizeType_t BucketCount;
		SizeType_t ElementCount;
		SizeType_t FurthestBucket;
		SizeType_t ClosestBucket;
		THash Hasher;
	};
}
