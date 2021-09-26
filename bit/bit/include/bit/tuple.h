#pragma once

#include <bit/container_allocators.h>

namespace bit
{
	namespace _
	{
		template<bit::SizeType_t Index, typename T>
		struct TupleGet {};
	}

	template<typename T, typename... TOthers>
	struct Tuple : public Tuple<TOthers...>
	{
		typedef Tuple<T, TOthers...> SelfType_t;
		typedef Tuple<TOthers...> BaseType_t;

		Tuple(T Value, TOthers... Others) :
			Tuple<TOthers...>(Others...),
			Value(Value)
		{}

		template<bit::SizeType_t Index>
		auto Get()
		{
			return _::TupleGet<Index, SelfType_t>::Get(*this);
		}

		T Value;
	};

	template<typename T>
	struct Tuple<T>
	{
		typedef Tuple<T> SelfType_t;
		typedef Tuple<T> BaseType_t;
		
		Tuple(T Value) :
			Value(Value)
		{}
		T Value;
	};
	namespace _
	{
		template<typename T, typename... TOthers>
		struct TupleGet<0, Tuple<T, TOthers...>>
		{
			static T& Get(Tuple<T, TOthers...>& Tuple)
			{
				return Tuple.Value;
			}
		};

		template<bit::SizeType_t Index, typename T, typename... TOthers>
		struct TupleGet<Index, Tuple<T, TOthers...>>
		{
			static auto Get(Tuple<T, TOthers...>& Tuple)
			{
				return TTupleGet<Index - 1, Tuple<TOthers...>>::Get(Tuple);
			}
		};
	}

	template<bit::SizeType_t Index, typename TTupleType>
	auto TGet(TTupleType& Tuple)
	{
		return _::TupleGet<Index, TTupleType>::Get(Tuple);
	}
}
