#pragma once

#include <bit/core/os/atomics.h>

namespace bit
{
	template<typename TCounterType>
	struct NonAtomicRefCounter
	{
		typedef NonAtomicRefCounter<TCounterType> SelfType_t;
		typedef TCounterType CounterType_t;

		NonAtomicRefCounter() : Counter(0) {}
		NonAtomicRefCounter(TCounterType Init) : Counter(Init) {}
		NonAtomicRefCounter(SelfType_t&& Move) :
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
		NonAtomicRefCounter(const SelfType_t&) = delete;
		SelfType_t& operator=(const SelfType_t&) = delete;
		TCounterType Counter;
	};

	template<typename TCounterType>
	struct AtomicRefCounter
	{
		typedef NonAtomicRefCounter<TCounterType> SelfType_t;
		typedef TCounterType CounterType_t;

		AtomicRefCounter() : Counter(0) {}
		AtomicRefCounter(TCounterType Init) : Counter(Init) {}
		AtomicRefCounter(SelfType_t&& Move) :
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
		AtomicRefCounter(const SelfType_t&) = delete;
		SelfType_t& operator=(const SelfType_t&) = delete;
		TCounterType Counter;
	};
}