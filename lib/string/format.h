#pragma once

#include <memory>
#include <cctype>
#include <variant>
#include <type_traits>

#include "lib/types.h"
#include "lib/string.h"
#include "lib/math/vector.h"

// format string
string va(stringlit fmt, ...);
string va(stringlit fmt, va_list list);

// convenience function to convert integer to string
inline string itos(int32_t v)
{
	return va("%i", v);
}

// convenience function to convert float to string
inline string ftos(float v)
{
	return va("%f", v);
};

// convenience function to convert vector to string
inline string vtos(vector v)
{
	return va("%f %f %f", v[0], v[1], v[2]);
};