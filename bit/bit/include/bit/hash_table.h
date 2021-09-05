#pragma once

#include <bit/types.h>
#include <bit/allocator.h>
#include <bit/murmur_hash.h>
#include <bit/memory.h>
#include <bit/linked_list.h>

namespace bit
{
	template<
		typename T,
		typename TSizeType,
		typename TBucketType,
		typename TBucketIteratorType
	>
	struct THashTableIterator
	{
		typedef THashTableIterator<T, TSizeType, TBucketType, TBucketIteratorType> SelfType_t;

		THashTableIterator(TBucketType* Buckets, TBucketIteratorType Iter, TSizeType BucketIndex, TSizeType BucketCount) :
			Buckets(Buckets),
			Iter(Iter),
			BucketIndex(BucketIndex),
			BucketCount(BucketCount)
		{}

		T& operator*()
		{
			return Iter->Data;
		}

		T* operator->()
		{
			if (Iter != Buckets[BucketIndex].Container.end())
				return &Iter->Data;
			return nullptr;
		}

		SelfType_t& operator++()
		{
			if (++Iter == Buckets[BucketIndex].Container.end())
			{
				for (TSizeType Index = BucketIndex + 1; Index < BucketCount; ++Index)
				{
					if (!Buckets[Index].Container.IsEmpty())
					{
						Iter = Buckets[Index].Container.begin();
						BucketIndex = Index;
						return *this;
					}
				}
				Iter.Invalidate();
			}
			return *this;
		}

		SelfType_t operator++(int32_t) { SelfType_t Self = *this; ++(*this); return Self; }

		friend bool operator==(const SelfType_t& A, const SelfType_t& B)
		{
			return A.Iter == B.Iter;
		}

		friend bool operator!=(const SelfType_t& A, const SelfType_t& B)
		{
			return A.Iter != B.Iter;
		}

	private:
		TBucketType* Buckets;
		TBucketIteratorType Iter;
		TSizeType BucketIndex;
		TSizeType BucketCount;
	};

	template<
		typename T,
		typename TSizeType,
		typename TBucketType,
		typename TBucketIteratorType
	>
	struct TConstHashTableIterator
	{
		typedef TConstHashTableIterator<T, TSizeType, TBucketType, TBucketIteratorType> SelfType_t;

		TConstHashTableIterator(TBucketType* Buckets, TBucketIteratorType Iter, TSizeType BucketIndex, TSizeType BucketCount) :
			Buckets(Buckets),
			Iter(Iter),
			BucketIndex(BucketIndex),
			BucketCount(BucketCount)
		{}

		const T& operator*()
		{
			return Iter->Data;
		}

		const T* operator->()
		{
			if (Iter != Buckets[BucketIndex].Container.cend())
				return &Iter->Data;
			return nullptr;
		}

		SelfType_t& operator++()
		{
			if (++Iter == Buckets[BucketIndex].Container.cend())
			{
				for (TSizeType Index = BucketIndex + 1; Index < BucketCount; ++Index)
				{
					if (!Buckets[Index].Container.IsEmpty())
					{
						Iter = Buckets[Index].Container.cbegin();
						BucketIndex = Index;
						return *this;
					}
				}
				Iter.Invalidate();
			}
			return *this;
		}

		SelfType_t operator++(int32_t) { SelfType_t Self = *this; ++(*this); return Self; }

		friend bool operator==(const SelfType_t& A, const SelfType_t& B)
		{
			return A.Iter == B.Iter;
		}

		friend bool operator!=(const SelfType_t& A, const SelfType_t& B)
		{
			return A.Iter != B.Iter;
		}

	private:
		const TBucketType* Buckets;
		TBucketIteratorType Iter;
		TSizeType BucketIndex;
		TSizeType BucketCount;
	};

	template<typename TKey, typename TValue>
	struct TKeyValue
	{
		typedef TKey KeyType_t;
		typedef TValue ValueType_t;
		KeyType_t Key;
		ValueType_t Value;
	};

	template<typename T, typename THashType>
	struct TBucketEntry
	{
		typedef T DataType_t;
		typedef THashType HashType_t;
		DataType_t Data;
		HashType_t Hash;
	};

	template<typename TBucketEntryContainer>
	struct TBucket
	{
		typedef typename TBucketEntryContainer::ElementType_t EntryType_t;
		typedef typename EntryType_t::DataType_t DataType_t;
		typedef typename EntryType_t::HashType_t HashType_t;
		typedef typename DataType_t::KeyType_t KeyType_t;
		typedef typename DataType_t::ValueType_t ValueType_t;
		typedef typename TBucketEntryContainer::SizeType_t SizeType_t;

		bool Erase(HashType_t Hash, const KeyType_t& Key)
		{
			EntryType_t* Entry = Find(Hash, Key);
			if (Entry != nullptr)
			{
				return Container.Erase(*Entry);
			}
			return false;
		}

		EntryType_t* Insert(HashType_t Hash, const KeyType_t& Key, const ValueType_t& Value)
		{
			return SetOrInsert(Hash, Key, Value);
		}

		EntryType_t* SetOrInsert(HashType_t Hash, const KeyType_t& Key, const ValueType_t& Value)
		{
			for (EntryType_t& Entry : Container)
			{
				if (Entry.Hash == Hash && Entry.Data.Key == Key)
				{
					return Set(&Entry, Hash, Key, Value);
				}
			}
			return CreateAndInsert(Hash, Key, Value);
		}

		EntryType_t* CreateAndInsert(HashType_t Hash, const KeyType_t& Key, const ValueType_t& Value)
		{
			return Set(Container.New(), Hash, Key, Value);
		}

		EntryType_t* Set(EntryType_t* Entry, HashType_t Hash, const KeyType_t& Key, const ValueType_t& Value)
		{
			Entry->Hash = Hash;
			Entry->Data.Key = Key;
			Entry->Data.Value = Value;
			return Entry;
		}

		EntryType_t* Find(HashType_t Hash, const KeyType_t& Key)
		{
			for (EntryType_t& Entry : Container)
			{
				if (Entry.Hash == Hash && Entry.Data.Key == Key)
				{
					return &Entry;
				}
			}
			return nullptr;
		}

		TBucketEntryContainer Container;
	};

	template<
		typename TKey,
		typename TValue,
		typename TSizeType = DefaultContainerSizeType_t,
		typename THashType = size_t,
		typename THash = TMurmurHash<TKey>,
		typename TAllocator = TDefaultHashTableAllocator<TSizeType>
	>
	struct THashTable
	{
		typedef THashTable<TKey, TValue, TSizeType, THashType, THash, TAllocator> SelfType_t;
		typedef typename TAllocator::BucketEntryAllocatorType_t BucketEntryAllocator_t;
		typedef typename TAllocator::BucketAllocatorType_t BucketAllocatorType_t;
		typedef TKeyValue<TKey, TValue> PairType_t;
		typedef TBucketEntry<PairType_t, THashType> BucketEntryType_t;
		typedef TLinkedList<BucketEntryType_t, TSizeType, BucketEntryAllocator_t> BucketEntryContainerType_t;
		typedef TBucket<BucketEntryContainerType_t> BucketType_t;
		typedef typename THashTableIterator<TKeyValue<TKey, TValue>, TSizeType, BucketType_t, typename BucketEntryContainerType_t::IteratorType_t> IteratorType_t;
		typedef typename TConstHashTableIterator<TKeyValue<TKey, TValue>, TSizeType, BucketType_t, typename BucketEntryContainerType_t::ConstIteratorType_t> ConstIteratorType_t;

	private:
		struct CTableKey
		{
			THashType Hash;
			TSizeType BucketIndex;
		};

	public:
		/* Begin range for loop implementation */
		IteratorType_t begin() { return IteratorType_t(Buckets, Buckets[ClosestBucket].Container.begin(), ClosestBucket, BucketCount); }
		IteratorType_t end() { return IteratorType_t(Buckets, Buckets[FurthestBucket].Container.end(), FurthestBucket, BucketCount); }
		ConstIteratorType_t cbegin() const { return ConstIteratorType_t(Buckets, Buckets[ClosestBucket].Container.cbegin(), ClosestBucket, BucketCount); }
		ConstIteratorType_t cend() const { return ConstIteratorType_t(Buckets, Buckets[FurthestBucket].Container.cend(), FurthestBucket, BucketCount); }
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
				BucketType_t* NewBuckets = (BucketType_t*)Allocator.Allocate(nullptr, sizeof(BucketType_t), NewSize);
				bit::ConstructArray<BucketType_t>(NewBuckets, NewSize);
				for (TSizeType Index = 0; Index < BucketCount; ++Index)
				{
					BucketType_t& Bucket = Buckets[Index];
					for (BucketEntryType_t& Entry : Bucket.Container)
					{
						TSizeType Index = Entry.Hash % NewSize;
						NewBuckets[Index].Container.Insert(Entry);
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
				BucketType_t* NewBuckets = (BucketType_t*)Allocator.Allocate(nullptr, sizeof(BucketType_t), NewSize);
				Buckets = bit::ConstructArray<BucketType_t>(NewBuckets, NewSize);
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
			return Buckets[TableKey.BucketIndex].Insert(TableKey.Hash, Key, Value)->Data.Value;
		}

		TValue& Insert(const TKey& Key, TValue&& Value)
		{
			CheckGrow();
			ElementCount += 1;
			CTableKey TableKey = GetTableKey(Key);
			FurthestBucket = bit::Max(FurthestBucket, TableKey.BucketIndex);
			ClosestBucket = bit::Min(ClosestBucket, TableKey.BucketIndex);
			return Buckets[TableKey.BucketIndex].Insert(TableKey.Hash, Key, bit::Move(Value))->Data.Value;
		}

		bool Erase(const TKey& Key)
		{
			CTableKey TableKey = GetTableKey(Key);
			BucketType_t& Bucket = Buckets[TableKey.BucketIndex];
			if (Bucket.Erase(TableKey.Hash, Key))
			{
				if (Bucket.Container.IsEmpty())
				{
					FurthestBucket = 0;
					ClosestBucket = BucketCount;
					for (TSizeType Index = 0; Index < TableKey.BucketIndex; ++Index)
					{
						if (!Buckets[Index].Container.IsEmpty())
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
			BucketEntryType_t* Entry = Buckets[TableKey.BucketIndex].Find(TableKey.Hash, Key);
			if (Entry != nullptr)
			{
				return Entry->Data.Value;
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

		BucketAllocatorType_t Allocator;
		BucketType_t* Buckets;
		TSizeType BucketCount;
		TSizeType ElementCount;
		TSizeType FurthestBucket;
		TSizeType ClosestBucket;
		THash Hasher;
	};

	namespace pmr
	{
		template<typename TBucketEntryContainer>
		struct TBucket
		{
			typedef typename TBucketEntryContainer::ElementType_t EntryType_t;
			typedef typename EntryType_t::DataType_t DataType_t;
			typedef typename EntryType_t::HashType_t HashType_t;
			typedef typename DataType_t::KeyType_t KeyType_t;
			typedef typename DataType_t::ValueType_t ValueType_t;
			typedef typename TBucketEntryContainer::SizeType_t SizeType_t;

			TBucket(bit::IAllocator* Allocator) :
				Container(*Allocator)
			{}

			bool Erase(HashType_t Hash, const KeyType_t& Key)
			{
				EntryType_t* Entry = Find(Hash, Key);
				if (Entry != nullptr)
				{
					return Container.Erase(*Entry);
				}
				return false;
			}

			EntryType_t* Insert(HashType_t Hash, const KeyType_t& Key, const ValueType_t& Value)
			{
				return SetOrInsert(Hash, Key, Value);
			}

			EntryType_t* SetOrInsert(HashType_t Hash, const KeyType_t& Key, const ValueType_t& Value)
			{
				for (EntryType_t& Entry : Container)
				{
					if (Entry.Hash == Hash && Entry.Data.Key == Key)
					{
						return Set(&Entry, Hash, Key, Value);
					}
				}
				return CreateAndInsert(Hash, Key, Value);
			}

			EntryType_t* CreateAndInsert(HashType_t Hash, const KeyType_t& Key, const ValueType_t& Value)
			{
				return Set(Container.Alloc(), Hash, Key, Value);
			}

			EntryType_t* Set(EntryType_t* Entry, HashType_t Hash, const KeyType_t& Key, const ValueType_t& Value)
			{
				Entry->Hash = Hash;
				Entry->Data.Key = Key;
				Entry->Data.Value = Value;
				return Entry;
			}

			EntryType_t* Find(HashType_t Hash, const KeyType_t& Key)
			{
				for (EntryType_t& Entry : Container)
				{
					if (Entry.Hash == Hash && Entry.Data.Key == Key)
					{
						return &Entry;
					}
				}
				return nullptr;
			}

			TBucketEntryContainer Container;
		};


		template<
			typename TKey,
			typename TValue,
			typename TSizeType = DefaultContainerSizeType_t,
			typename THashType = size_t,
			typename THash = TMurmurHash<TKey>
		>
		struct THashTable
		{
			typedef THashTable<TKey, TValue, TSizeType, THashType, THash> SelfType_t;
			typedef TKeyValue<TKey, TValue> PairType_t;
			typedef TBucketEntry<PairType_t, THashType> BucketEntryType_t;
			typedef TLinkedList<BucketEntryType_t, TSizeType> BucketEntryContainerType_t;
			typedef TBucket<BucketEntryContainerType_t> BucketType_t;
			typedef typename THashTableIterator<TKeyValue<TKey, TValue>, TSizeType, BucketType_t, typename BucketEntryContainerType_t::IteratorType_t> IteratorType_t;
			typedef typename TConstHashTableIterator<TKeyValue<TKey, TValue>, TSizeType, BucketType_t, typename BucketEntryContainerType_t::ConstIteratorType_t> ConstIteratorType_t;


		private:
			struct CTableKey
			{
				THashType Hash;
				TSizeType BucketIndex;
			};

		public:

			/* Begin range for loop implementation */
			IteratorType_t begin() { return IteratorType_t(Buckets, Buckets[ClosestBucket].Container.begin(), ClosestBucket, BucketCount); }
			IteratorType_t end() { return IteratorType_t(Buckets, Buckets[FurthestBucket].Container.end(), FurthestBucket, BucketCount); }
			ConstIteratorType_t cbegin() const { return ConstIteratorType_t(Buckets, Buckets[ClosestBucket].Container.cbegin(), ClosestBucket, BucketCount); }
			ConstIteratorType_t cend() const { return ConstIteratorType_t(Buckets, Buckets[FurthestBucket].Container.cend(), FurthestBucket, BucketCount); }
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
					BucketType_t* NewBuckets = Allocator->Allocate<BucketType_t>(NewSize);
					bit::ConstructArray<BucketType_t>(NewBuckets, NewSize, Allocator);
					for (TSizeType Index = 0; Index < BucketCount; ++Index)
					{
						BucketType_t& Bucket = Buckets[Index];
						for (BucketEntryType_t& Entry : Bucket.Container)
						{
							TSizeType Index = Entry.Hash % NewSize;
							NewBuckets[Index].Container.Insert(Entry);
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
					Buckets = bit::ConstructArray<BucketType_t>(Allocator->Allocate<BucketType_t>(NewSize), NewSize, Allocator);
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
				return Buckets[TableKey.BucketIndex].Insert(TableKey.Hash, Key, Value)->Data.Value;
			}

			TValue& Insert(const TKey& Key, TValue&& Value)
			{
				CheckGrow();
				ElementCount += 1;
				CTableKey TableKey = GetTableKey(Key);
				FurthestBucket = bit::Max(FurthestBucket, TableKey.BucketIndex);
				ClosestBucket = bit::Min(ClosestBucket, TableKey.BucketIndex);
				return Buckets[TableKey.BucketIndex].Insert(TableKey.Hash, Key, bit::Move(Value))->Data.Value;
			}

			bool Erase(const TKey& Key)
			{
				CTableKey TableKey = GetTableKey(Key);
				BucketType_t& Bucket = Buckets[TableKey.BucketIndex];
				if (Bucket.Erase(TableKey.Hash, Key))
				{
					if (Bucket.Container.IsEmpty())
					{
						FurthestBucket = 0;
						ClosestBucket = BucketCount;
						for (TSizeType Index = 0; Index < TableKey.BucketIndex; ++Index)
						{
							if (!Buckets[Index].Container.IsEmpty())
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
				BucketEntryType_t* Entry = Buckets[TableKey.BucketIndex].Find(TableKey.Hash, Key);
				if (Entry != nullptr)
				{
					return Entry->Data.Value;
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
			BucketType_t* Buckets;
			TSizeType BucketCount;
			TSizeType ElementCount;
			TSizeType FurthestBucket;
			TSizeType ClosestBucket;
			THash Hasher;
		};
	}
}
