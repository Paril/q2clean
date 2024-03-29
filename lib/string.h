#pragma once

#include "lib/std.h"
#include "lib/types.h"
#include "lib/types/allocator.h"
#include "lib/types/array.h"
#include "lib/types/dynarray.h"
#include "lib/math.h"

// type name for a string literal
using stringlit = const char *;

class stringref;
class string;

// a mutable string, which can be expanded and formatted to.
// this is the preferred class to use for a string that is
// changing around, but not shared across multiple instances.
class mutable_string : public std::basic_string<char, std::char_traits<char>, game_allocator<char>>
{
	using basic_string::basic_string;
};

// most strings in Q2++ are immutable. this is to remain simple
// and allow them to be collected automatically via a smart pointer.
class string
{
	using ptr_type = std::shared_ptr<char[]>;

	std::variant<ptr_type, mutable_string> shared;
	size_t slength;

	// copy string literal passed by argument into a new string.
	// internal; length is not null terminated
	inline string(stringlit lit, size_t length) :
		shared(),
		slength(length)
	{
		if (slength)
		{
			auto ptr = std::allocate_shared<char[]>(game_allocator<char[]>(), slength + 1);

			if (lit)
			{
				memcpy(ptr.get(), lit, slength);
				ptr[slength] = 0;
			}
			else
				ptr[0] = 0;

			shared = std::move(ptr);
		}
		else
			shared = ptr_type(nullptr);
	}

public:
	inline string() :
		string(nullptr, 0)
	{
	}

	// copy a string from a mutable string
	inline string(const mutable_string &string) :
		string(string.data(), string.length())
	{
	}
	
	// move a mutable string into a saved string
	inline string(mutable_string &&string) noexcept :
		shared(std::move(string)),
		slength(std::get<mutable_string>(shared).size())
	{
	}

	// share string passed by argument into this string
	inline string(const string &share) :
		shared(share.shared),
		slength(share.slength)
	{
	}

	// copy string literal passed by argument into a new string
	inline string(stringlit lit) :
		string(lit, lit ? strlen(lit) : 0)
	{
	}

	// copy string ref passed by argument into a new string
	inline string(const stringref &ref);

	// copy substring literal passed by argument into a new string.
	// mainly internal; start/length must be validated before calling this
	inline string(stringlit sub, const size_t &start, const size_t &length) :
		string(length ? (sub + start) : 0, length)
	{
	}

	// copy substring ref passed by argument into a new string.
	// mainly internal; start/length must be validated before calling this
	inline string(const stringref &sub, const size_t &start, const size_t &length);

	// allocate new string with specified length. used internally.
	explicit inline string(const size_t &length) :
		string(nullptr, length)
	{
	}

	// get underlying string literal.
	// don't store these, as this could be bad pointer
	// once it expires. this is mainly for interop with 
	// C library stuff.
	inline explicit operator stringlit() const
	{
		if (std::holds_alternative<ptr_type>(shared))
			return std::get<ptr_type>(shared).get();
		return std::get<mutable_string>(shared).data();
	}
	
	// get underlying string literal.
	// don't store these, as this could be bad pointer
	// once it expires. this is mainly for interop with 
	// C library stuff.
	inline stringlit ptr() const
	{
		return this->operator stringlit();
	}
	
	inline size_t length() const { return slength; }
	inline size_t size() const { return slength; }

	inline const char &operator[](const size_t &index) const
	{
		static char zerochar = 0;
		auto p = ptr();

		if (!p || (size_t) index >= slength)
			return zerochar;

		return p[index];
	}

	inline bool operator==(stringlit lit) const
	{
		// both literals null or empty
		if (!*this && (!lit || !*lit))
			return true;
		// only one side is empty
		else if (!*this || (!lit || !*lit))
			return false;

		return !strcmp(ptr(), lit);
	}
	inline bool operator!=(stringlit lit) const { return !(*this == lit); }

	inline bool operator==(const string &lit) const { return *this == lit.ptr(); }
	inline bool operator!=(const string &lit) const { return !(*this == lit); }

	inline bool operator==(const stringref &lit) const;
	inline bool operator!=(const stringref &lit) const;

	// string is "valid" if its non-null and doesn't start with a zero
	inline explicit operator bool() const { return ptr() && slength; }
};

// faster strlen for strings
inline size_t strlen(const string &str)
{
	return str.length();
}

// faster strlen for mutable strings
inline size_t strlen(const mutable_string &str)
{
	return str.length();
}

// stringref is a special wrapper to string types
// that can keep them from expiring too early.
// note that this can't keep mutable_string's alive.
class stringref
{
	using variantref = std::variant<stringlit, string>;
	variantref data;
	size_t slength;

public:
	constexpr stringref() :
		data(nullptr),
		slength(0)
	{
	}

	inline stringref(string &&str) :
		data(std::move(str)),
		slength(std::get<string>(data).length())
	{
	}
	
	// move a mutable string into a stringref
	inline stringref(mutable_string &&str) noexcept :
		data(std::move(str)),
		slength(std::get<string>(data).length())
	{
	}

	inline stringref(const string &str) :
		data(str),
		slength(str.length())
	{
	}

	inline stringref(stringlit lit) :
		data(lit),
		slength(lit ? strlen(lit) : 0)
	{
	}

	inline stringref(const mutable_string &str) :
		data(str.data()),
		slength(str.length())
	{
	}

	// convert to ptr; don't store this outside of string's
	// lifetime!
	inline explicit operator stringlit() const
	{
		if (std::holds_alternative<string>(data))
			return (stringlit) std::get<string>(data);
		return std::get<stringlit>(data);
	}
	
	// convert to ptr; don't store this outside of string's
	// lifetime!
	inline stringlit ptr() const { return operator stringlit(); }
	
	inline size_t length() const { return slength; }
	inline size_t size() const { return slength; }

	inline const char &operator[](const size_t &index) const
	{
		static char zerochar = 0;
		auto ptr = operator stringlit();

		if (!ptr || index >= slength)
			return zerochar;

		return ptr[index];
	}

	inline bool operator==(stringlit lit) const
	{
		// both literals null or empty
		if (!*this && (!lit || !*lit))
			return true;
		// only one side is empty
		else if (!*this || (!lit || !*lit))
			return false;

		return !strcmp(ptr(), lit);
	}
	inline bool operator!=(stringlit lit) const { return !(*this == lit); }

	inline bool operator==(const stringref &lit) const { return *this == lit.ptr(); }
	inline bool operator!=(const stringref &lit) const { return !(*this == lit); }

	// string is "valid" if its length is non-zero and doesn't start with a 0
	inline explicit operator bool() const { return slength && operator stringlit() && operator stringlit()[0]; }
};

inline bool string::operator==(const stringref &lit) const { return *this == lit.ptr(); }
inline bool string::operator!=(const stringref &lit) const { return !(*this == lit); }

template<typename T>
constexpr bool is_string_v = std::is_same_v<T, stringref> || std::is_same_v<T, string> || std::is_same_v<T, stringlit> || (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>);

template<typename T>
constexpr bool is_string_not_literal_v = std::is_same_v<T, stringref> || std::is_same_v<T, string>;

template<typename T>
concept is_string = is_string_v<T>;

template<typename T>
concept is_string_not_literal = is_string_not_literal_v<T>;

// copy string ref passed by argument into a new string
inline string::string(const stringref &ref) :
	string(ref.ptr(), ref.length())
{
}

// copy substring ref passed by argument into a new string.
// mainly internal; start/length must be validated before calling this
inline string::string(const stringref &sub, const size_t &start, const size_t &length) :
	string(length ? (sub.ptr() + start) : nullptr, length)
{
}

// return index of substring in str, or -1
template<is_string_not_literal TA, is_string TB>
inline size_t strstr(const TA &str, const TB &substring)
{
	if (!str)
		return (size_t) -1;

	stringlit result = strstr(str.ptr(), (stringlit)substring);
	return result ? (result - str.ptr()) : -1;
}

// return index of substring in str, or -1
template<is_string_not_literal T>
inline size_t strchr(const T &str, const char &subchar)
{
	if (!str)
		return (size_t)-1;

	stringlit result = strchr(str.ptr(), (int)subchar);
	return result ? (result - str.ptr()) : -1;
}

// return substring of str
template<is_string_not_literal T>
inline string substr(const T &str, const size_t &start, size_t length = -1)
{
	// string too big; return empty string
	if (start >= str.length())
		return string();

	// clip the length
	length = min(length, str.length() - start);

	// no string left to clip
	if (!length)
		return string();

	return string(str, start, length);
}

// do concat!
inline string strconcat(const std::initializer_list<stringref> &args)
{
	if (!args.size())
		return "";
	else if (args.size() == 1)
		return *args.begin();

	size_t len = 0;

	for (auto &ref : args)
		len += ref.length();

	string outstr(len);
	char *out = const_cast<char *>((stringlit)outstr);

	for (auto &ref : args)
	{
		len = ref.length();

		if (!len)
			continue;

		memcpy(out, (stringlit)ref, len);
		out += len;
	}

	*out = 0;
	return outstr;
}

template<is_string ...T>
inline string strconcat(T... args)
{
	std::initializer_list<stringref> refs = { args... };
	return strconcat(refs);
}

// faster strlen for stringrefs
template<is_string_not_literal T>
inline size_t strlen(const T &str)
{
	return str.length();
}

template<is_string_not_literal T>
inline double atof(const T &str)
{
	if (!str.ptr())
		return 0;

	return atof(str.ptr());
}

template<is_string_not_literal T>
inline double atoi(const T &str)
{
	if (!str.ptr())
		return 0;

	return atoi(str.ptr());
}

// convert string to lowercase
template<is_string T>
inline string strlwr(const T &str)
{
	string s(str);

	char *out = const_cast<char *>(s.ptr());
	char *end = out + s.length();

	for (; out != end; out++)
		*out = (char)tolower(*out);

	return s;
}

// convert string to uppercase
template<is_string T>
inline string strupr(const T &str)
{
	string s(str);

	char *out = const_cast<char *>(s.ptr());
	char *end = out + s.length();

	for (; out != end; out++)
		*out = (char)toupper(*out);

	return s;
}

template<is_string TA, is_string TB>
inline int32_t strcmp(const TA &a, const TB &b)
{
	return ::strcmp((stringlit) a, (stringlit) b);
}

template<is_string TA, is_string TB>
inline int32_t strncmp(const TA &a, const TB &b, const size_t &max_count)
{
	return ::strncmp((stringlit) a, (stringlit) b, max_count);
}

// case insensitive comparison (because stricmp/strnicmp is non-standard)
template<is_string TA, is_string TB>
inline int32_t stricmp(const TA &a, const TB &b)
{
	stringlit al = (stringlit) a;
	stringlit bl = (stringlit) b;

	while (toupper(*al) == toupper(*bl))
	{
		if (*al == 0)
			return 0;
		al++;
		bl++;
	}

	return toupper(*al) - toupper(*bl);
}

// case insensitive comparison (because stricmp/strnicmp is non-standard)
template<is_string TA, is_string TB>
inline int32_t strnicmp(const TA &a, const TB &b, size_t n)
{
	if (!n)
		return 0;

	stringlit al = (stringlit) a;
	stringlit bl = (stringlit) b;
	
	do
	{
		if (toupper(*al) != toupper(*bl++))
			return toupper(*al) - toupper(*--bl);
		else if (*al++ == 0)
			break;
	} while (--n != 0);

	return 0;
}

// compatibility
template<is_string_not_literal T>
inline bool strempty(const T &a)
{
	return !a;
}

// case insensitive ==
template<is_string TA, is_string TB>
inline bool striequals(const TA &a, const TB &b)
{
	if (!a && !b)
		return true;
	else if (!a || !b)
		return false;
	
	stringlit la = (stringlit)a;
	stringlit lb = (stringlit)b;

	while (tolower(*la) == tolower(*lb))
	{
		if (*la == 0)
			return true;

		la++;
		lb++;
	}

	return false;
}

// stringarray is a special type mainly used for interop,
// but basically it's a static array of characters.
template<size_t size>
class stringarray : public array<char, size>
{
public:
	constexpr stringarray() :
		array<char, size>({ '\0' })
	{
	}

	constexpr stringarray(stringlit lit)
	{
		if (!lit)
		{
			this->data()[0] = '\0';
			return;
		}

		const size_t len = strlen(lit);
		size_t i = 0;

		for (; i < min(len, size - 1); i++)
			this->data()[i] = lit[i];

		this->data()[i] = 0;
	}

	constexpr stringarray(const array<char, size> &init) :
		array<char, size>(init)
	{
	}

	constexpr operator stringlit() const
	{
		return this->data();
	}

	inline bool operator==(stringlit lit) const { return !strcmp(operator stringlit(), lit); }
	inline bool operator!=(stringlit lit) const { return !(*this == lit); }

	inline bool operator==(const stringref &lit) const { return !strcmp(operator stringlit(), lit.ptr()); }
	inline bool operator!=(const stringref &lit) const { return !(*this == lit); }

	// string is "valid" if its length is non-zero and doesn't start with a 0
	inline explicit operator bool() const { return size && operator stringlit()[0]; }
};

/*
==============
strtok

Parse a token out of a string.
Handles C and C++ comments.
==============
*/
inline string strtok(const stringref &data, size_t &start)
{
	size_t token_start = start, token_end;

	if (!data[0])
	{
		start = (size_t)-1;
		return "";
	}

// skip whitespace
	char c;

skipwhite:
	while ((c = data[token_start]) <= ' ')
	{
		if (c == 0)
		{
			start = (size_t)-1;
			return "";
		}
		token_start++;
	}

// skip // comments
	if (c == '/' && data[token_start + 1] == '/')
	{
		token_start += 2;

		while ((c = data[token_start]) && c != '\n')
			token_start++;
		
		goto skipwhite;
	}

// skip /* */ comments
	if (c == '/' && data[token_start + 1] == '*')
	{
		token_start += 2;
		while ((c = data[token_start]))
		{
			if (c == '*' && data[token_start + 1] == '/')
			{
				token_start += 2;
				break;
			}
			token_start++;
		}
		goto skipwhite;
	}

	token_end = token_start;

// handle quoted strings specially
	if (c == '\"')
	{
		token_end++;
		while ((c = data[token_end++]) && c != '\"') ;

		if (!data[token_end])
			start = (size_t)-1;
		else
			start = token_end;

		return substr(data, token_start + 1, token_end - token_start - 2);
	}

// parse a regular word
	do
	{
		token_end++;
		c = data[token_end];
	} while (c > 32);

	if (!data[token_end])
		start = (size_t)-1;
	else
		start = token_end;

	return substr(data, token_start, token_end - token_start);
}