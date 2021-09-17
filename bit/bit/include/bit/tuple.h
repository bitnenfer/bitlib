#pragma once

#include <bit/container_allocators.h>

namespace bit
{
	namespace _
	{
		template<bit::SizeType_t Index, typename T>
		struct TTupleGet {};
	}

	template<typename T, typename... TOthers>
	struct TTuple : public TTuple<TOthers...>
	{
		typedef TTuple<T, TOthers...> SelfType_t;
		typedef TTuple<TOthers...> BaseType_t;

		TTuple(T Value, TOthers... Others) :
			TTuple<TOthers...>(Others...),
			Value(Value)
		{}

		template<bit::SizeType_t Index>
		auto Get()
		{
			return _::TTupleGet<Index, SelfType_t>::Get(*this);
		}

		T Value;
	};

	template<typename T>
	struct TTuple<T>
	{
		typedef TTuple<T> SelfType_t;
		typedef TTuple<T> BaseType_t;
		
		TTuple(T Value) :
			Value(Value)
		{}
		T Value;
	};
	namespace _
	{
		template<typename T, typename... TOthers>
		struct TTupleGet<0, TTuple<T, TOthers...>>
		{
			static T& Get(TTuple<T, TOthers...>& Tuple)
			{
				return Tuple.Value;
			}
		};

		template<bit::SizeType_t Index, typename T, typename... TOthers>
		struct TTupleGet<Index, TTuple<T, TOthers...>>
		{
			static auto Get(TTuple<T, TOthers...>& Tuple)
			{
				return TTupleGet<Index - 1, TTuple<TOthers...>>::Get(Tuple);
			}
		};
	}

	template<bit::SizeType_t Index, typename TTupleType>
	auto TGet(TTupleType& Tuple)
	{
		return _::TTupleGet<Index, TTupleType>::Get(Tuple);
	}
}
