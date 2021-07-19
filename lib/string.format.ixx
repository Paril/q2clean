module;

#include "types.h"

#include <memory>
#include <cctype>
#include <variant>
#include <type_traits>

export module string.format;

import string;
import math.vector;

// format string
export string va(stringlit fmt, ...);
export string va(stringlit fmt, va_list list);

// convenience function to convert integer to string
export inline string itos(int32_t v)
{
	return va("%i", v);
}

// convenience function to convert float to string
export inline string ftos(float v)
{
	return va("%f", v);
};

// convenience function to convert vector to string
export inline string vtos(vector v)
{
	return va("%f %f %f", v[0], v[1], v[2]);
};