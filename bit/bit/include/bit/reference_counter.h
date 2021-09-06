#pragma once

#include <bit/atomics.h>

namespace bit
{
	template<typename TCounterType>
	struct TNonAtomicRefCounter
	{
		typedef TNonAtomicRefCounter<TCounterType> SelfType_t;
		typedef TCounterType CounterType_t;

		TNonAtomicRefCounter() : Counter(0) {}
		TNonAtomicRefCounter(TCounterType Init) : Counter(Init) {}
		TNonAtomicRefCounter(SelfType_t&& Move) :
			Counter(Move.Counter)
		{
			Move.Counter = 0;
		}

		SelfType_t& operator=(SelfType_t&& Move)
		{
			Counter = Move.Counter;
			Move.Counter = 0;
			return *this;
		}

		void Reset()
		{
			Counter = 0;
		}

		void Increment()
		{
			++Counter;
		}

		bool Decrement()
		{
			return --Counter == 0;
		}

		bool GetCount() const { return Counter; }

	private:
		TNonAtomicRefCounter(const SelfType_t&) = delete;
		SelfType_t& operator=(const SelfType_t&) = delete;
		TCounterType Counter;
	};

	template<typename TCounterType>
	struct TAtomicRefCounter
	{
		typedef TNonAtomicRefCounter<TCounterType> SelfType_t;
		typedef TCounterType CounterType_t;

		TAtomicRefCounter() : Counter(0) {}
		TAtomicRefCounter(TCounterType Init) : Counter(Init) {}
		TAtomicRefCounter(SelfType_t&& Move) :
			Counter(Move.Counter)
		{
			bit::AtomicExchange(&Counter, Move.Counter);
			bit::AtomicExchange(Move.Counter, 0);
		}

		SelfType_t& operator=(SelfType_t&& Move)
		{
			bit::AtomicExchange(&Counter, Move.Counter);
			bit::AtomicExchange(Move.Counter, 0);
			return *this;
		}

		void Reset()
		{
			bit::AtomicExchange(&Counter, 0);
		}

		void Increment()
		{
			bit::AtomicIncrement(&Counter);
		}

		bool Decrement()
		{
			return bit::AtomicDecrement(&Counter) == 0;
		}

		bool GetCount() const { return Counter; }

	private:
		TAtomicRefCounter(const SelfType_t&) = delete;
		SelfType_t& operator=(const SelfType_t&) = delete;
		TCounterType Counter;
	};
}