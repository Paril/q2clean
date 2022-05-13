#pragma once

#include "lib/std.h"
#include "lib/types.h"
#include "lib/string.h"
#include "lib/math/vector.h"

// new format string!

// format the specified string, using the C++20 format lib.
// returns a mutable string, which you can convert into other
// strings types, move it.
template<typename T, typename ...Args>
inline mutable_string format(T &&fmt, Args &&...args)
{
	mutable_string buffer;
	std::vformat_to(std::back_inserter(buffer), std::forward<T>(fmt), std::make_format_args(args...));
	return buffer;
}

template<typename T, typename ...Args>
inline mutable_string &format_to(mutable_string &str, T &&fmt, Args &&...args)
{
	std::vformat_to(std::back_inserter(str), std::forward<T>(fmt), std::make_format_args(args...));
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