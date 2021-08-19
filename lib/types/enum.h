#pragma once

#include "lib/std.h"

// certain enums used in the code are bitwise. this allows bitwise
// operators to work. stupid but functional.
#define MAKE_ENUM_BITWISE(enumtype) \
	constexpr enumtype operator|(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l | (std::underlying_type<enumtype>::type)r); } \
	constexpr enumtype operator&(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l & (std::underlying_type<enumtype>::type)r); } \
	constexpr enumtype operator^(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l ^ (std::underlying_type<enumtype>::type)r); } \
	constexpr enumtype operator~(const enumtype &v) { return (enumtype)(~(std::underlying_type<enumtype>::type)v); } \
	constexpr enumtype &operator|=(enumtype &l, const enumtype &r) { return l = l | r; } \
	constexpr enumtype &operator&=(enumtype &l, const enumtype &r) { return l = l & r; } \
	constexpr enumtype &operator^=(enumtype &l, const enumtype &r) { return l = l ^ r; }
