#pragma once

#include <type_traits>

// certain enums used in the code are bitwise. this allows bitwise
// operators to work. stupid but functional.
#define MAKE_ENUM_BITWISE(enumtype, ...) \
	__VA_ARGS__ constexpr enumtype operator|(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l | (std::underlying_type<enumtype>::type)r); } \
	__VA_ARGS__ constexpr enumtype operator&(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l & (std::underlying_type<enumtype>::type)r); } \
	__VA_ARGS__ constexpr enumtype operator^(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l ^ (std::underlying_type<enumtype>::type)r); } \
	__VA_ARGS__ constexpr enumtype operator~(const enumtype &v) { return (enumtype)(~(std::underlying_type<enumtype>::type)v); } \
	__VA_ARGS__ constexpr enumtype &operator|=(enumtype &l, const enumtype &r) { return l = l | r; } \
	__VA_ARGS__ constexpr enumtype &operator&=(enumtype &l, const enumtype &r) { return l = l & r; } \
	__VA_ARGS__ constexpr enumtype &operator^=(enumtype &l, const enumtype &r) { return l = l ^ r; }
