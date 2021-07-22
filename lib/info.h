#pragma once

import "lib/gi.h";
import "lib/string.h";

constexpr bool isprint(const char &c)
{
	return c >= 0x20 && c <= 0x7E;
};

constexpr size_t MAX_INFO_KEY		= 64;
constexpr size_t MAX_INFO_VALUE	= 64;
constexpr size_t MAX_INFO_STRING	= 512;

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
string Info_ValueForKey(const stringref &s, stringlit key);

/*
==================
Info_RemoveKey
==================
*/
string Info_RemoveKey(const stringref &s, stringlit key);

/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing.
Also checks the length of keys/values and the whole string.
==================
*/
bool Info_Validate(const stringref &s);

/*
==================
Info_SetValueForKey
==================
*/
bool Info_SetValueForKey(string &s, stringlit key, stringlit value);
