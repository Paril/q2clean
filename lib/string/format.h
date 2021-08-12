#pragma once

#include <memory>
#include <cctype>
#include <variant>
#include <type_traits>

#include "lib/types.h"
#include "lib/string.h"
#include "lib/math/vector.h"

// new format string!
#include <format>

// format the specified string, using the C++20 format lib.
// returns a mutable string, which you can convert into other
// strings types, move it.
template<typename ...Args>
inline mutable_string format(stringlit fmt, const Args &...args)
{
	mutable_string buffer;
	std::format_to(std::back_inserter(buffer), fmt, std::forward<const Args>(args)...);
	return buffer;
}

template<typename ...Args>
inline mutable_string &format_to(mutable_string &str, stringlit fmt, const Args &...args)
{
	std::format_to(std::back_inserter(str), fmt, std::forward<const Args>(args)...);
	return str;
}

template<typename ...Args>
inline mutable_string &format_to_n(mutable_string &str, size_t max, stringlit fmt, const Args &...args)
{
	std::format_to_n(std::back_inserter(str), max, fmt, std::forward<const Args>(args)...);
	return str;
}

// formatter for string and stringref
template<>
struct std::formatter<string> : std::formatter<stringlit>
{
	template<typename FormatContext>
	auto format(const ::string &s, FormatContext &ctx)
	{
		return std::formatter<stringlit>::format(s.ptr(), ctx);
	}
};

template<>
struct std::formatter<stringref> : std::formatter<stringlit>
{
	template<typename FormatContext>
	auto format(const stringref &s, FormatContext &ctx)
	{
		return std::formatter<stringlit>::format(s.ptr(), ctx);
	}
};

template<>
struct std::formatter<vector> : std::formatter<float>
{
	template<typename FormatContext>
	auto format(const ::vector &v, FormatContext &ctx)
	{
		// FIXME: shim in the arguments from base formatter
		return std::vformat_to(ctx.out(), "{} {} {}", std::make_format_args(v.x, v.y, v.z));
	}
};