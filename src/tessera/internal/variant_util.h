#pragma once

#include <vector>
#include <variant>
#include <stdexcept>
#include <iterator>
#include <algorithm>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;

template<typename T, typename... Ts, typename... Vs>
bool is_one_of(const std::variant<Vs...>& v) {
	if (std::holds_alternative<T>(v))
		return true;
	if constexpr (sizeof...(Ts) != 0)
		return is_one_of<Ts...>(v);
	else
		return false;
}

template <class... Args>
struct variant_cast_proxy
{
	std::variant<Args...> v;

	template <class... ToArgs>
	operator std::variant<ToArgs...>() const
	{
		return std::visit(
			[](auto&& arg) -> std::variant<ToArgs...> {
				if constexpr (std::is_convertible_v<decltype(arg), std::variant<ToArgs...>>)
					return arg;
				else
					throw std::runtime_error("bad variant cast");
			},
			v
		);
	}
};

template <class... Args>
auto variant_cast(const std::variant<Args...>& v) -> variant_cast_proxy<Args...>
{
	return { v };
}

template <class... Args>
auto variant_cast( std::variant<Args...>&& v) -> variant_cast_proxy<Args...>
{
	return { std::move(v) };
}

template<typename T, typename Iter>
std::vector<T> get_vector(Iter beg, Iter end) {
	std::vector<T> output;
	std::transform(beg, end, std::back_inserter(output),
		[](const auto& v)->T {
			return std::get<T>(v);
		}
	);
	return output;
}
