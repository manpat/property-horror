#pragma once

#include <fmt/format.h>
#include <type_traits>
#include <cstdint>
#include <string>
#include <new>
#include <utility>
#include <string_view>


template<class...>
[[maybe_unused]] static constexpr bool false_v = false;

template<class S>
concept StandardLayout = std::is_standard_layout_v<S>;
template<class S>
concept Enum = std::is_enum_v<S>;

template <class From, class To>
concept convertible_to =
	std::is_convertible_v<From, To> &&
	requires(std::add_rvalue_reference_t<From> (&f)()) {
		static_cast<To>(f());
	};


// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0908r0.html :(
// https://gist.github.com/graphitemaster/494f21190bb2c63c5516
// template <StandardLayout S, typename T>
// constexpr uintptr_t member_offsetof(T S::* member) {
// 	alignas(S) char container_space[sizeof(S)] = {};
// 	auto const fake_container = std::launder(reinterpret_cast<S const*>(container_space));
// 	// despite std::launder I believe this is still UB
// 	// for non-trivial types, fake_container isn't safe to access as it's lifetime doesn't begin until
// 	// its initialisation completes [basic.life]. this is important bc unary operator& seems to require
// 	// its target is a valid object [expr.unary.op] (how dare it) 
// 	return reinterpret_cast<uintptr_t>(&(fake_container->*member)) - reinterpret_cast<uintptr_t>(fake_container);
// }



template<class T>
concept DebugFormattable = requires(T const& t, fmt::memory_buffer buf) {
	{ format_debug(t) } -> convertible_to<std::string>;
};

template<DebugFormattable T>
struct fmt::formatter<T> {
	constexpr auto parse(format_parse_context& ctx) {
		return std::find(ctx.begin(), ctx.end(), '}');
	}

	template <typename FormatContext>
	auto format(T const& t, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), format_debug(t));
	}
};



template<class Pattern>
std::pair<std::string_view, std::string_view> split(std::string_view s, Pattern pat) {
	auto const sep_idx = s.find(pat);
	auto const first_half = s.substr(0, sep_idx);
	s.remove_prefix(std::min(first_half.size()+1, s.size()));

	return {first_half, s};
}




