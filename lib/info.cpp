import "config.h";
#include "info.h"

import protocol;
import string.format;

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
string Info_ValueForKey(const stringref &s, stringlit key)
{
	string key_string = va("\\%s\\", key);
	size_t key_start = strstr(s, key_string);
	
	if (key_start == -1)
		return "";

	size_t value_start = key_start + strlen(key_string), value_end = value_start + 1;
	
	while (true)
	{
		char c = s[value_end];
		
		if (c == '\\' || !c)
			break;
		
		value_end++;
	}

	return substr(s, value_start, value_end - value_start);
};

/*
==================
Info_RemoveKey
==================
*/
string Info_RemoveKey(const stringref &s, stringlit key)
{
	string result = s;
	string key_string = va("\\%s\\", key);
	
	size_t key_start;
	while ((key_start = strstr(result, key_string)) != -1)
	{
		size_t value_end = key_start + strlen(key_string) + 1;
		
		while (true)
		{
			char c = result[value_end];
			
			if (c == '\\' || !c)
				break;
			
			value_end++;
		}
		
		string left = "", right = "";
		
		if (key_start != 0)
			left = substr(result, 0, key_start);
		if (value_end != strlen(result))
			right = substr(result, value_end, strlen(result) - value_end);
		
		result = strconcat(left, right);
	}
	
	return result;
}

/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing.
Also checks the length of keys/values and the whole string.
==================
*/
bool Info_Validate(const stringref &s)
{
	size_t total = 0, offset = 0;

	while (1)
	{
		//
		// validate key
		//
		if (s[offset] == '\\')
		{
			if (++total == MAX_INFO_STRING)
				return false;   // oversize infostring

			offset++;
		}
		if (!s[offset])
			return false;   // missing key

		int len = 0;
		while (s[offset] != '\\')
		{
			char c = s[offset++];
			if (c & 128 || !isprint(c) || c == '\"' || c == ';')
				return false;   // illegal characters
			else if (++len == MAX_INFO_KEY)
				return false;   // oversize key
			else if (++total == MAX_INFO_STRING)
				return false;   // oversize infostring
			else if (!s[offset])
				return false;   // missing value
		}

		//
		// validate value
		//
		offset++;
		if (++total == MAX_INFO_STRING)
			return false;   // oversize infostring
		if (!s[offset])
			return false;   // missing value

		len = 0;
		while (s[offset] != '\\')
		{
			int c = s[offset++];
			if (c & 128 || !isprint(c) || c == '\"' || c == ';')
				return false;   // illegal characters
			else if (++len == MAX_INFO_VALUE)
				return false;   // oversize value
			else if (++total == MAX_INFO_STRING)
				return false;   // oversize infostring
			else if (!s[offset])
				return true;    // end of string
		}
	}
	
	return true;
}

/*
============
Info_SubValidate
============
*/
static size_t Info_SubValidate(const stringref &s)
{
	size_t len = 0, offset = 0;

	while (s[offset])
	{
		char c = s[offset++];
		if (c & 128 || !isprint(c) || c == '\\' || c == '\"' || c == ';')
			return INT_MAX;  // illegal characters
		else if (++len == MAX_QPATH)
			return MAX_QPATH;  // oversize value
	}

	return len;
}

/*
==================
Info_SetValueForKey
==================
*/
bool Info_SetValueForKey(string &s, stringlit key, stringlit value)
{
	// validate key
	size_t kl = Info_SubValidate(key);
	if (kl >= MAX_QPATH)
		return false;

	// validate value
	size_t vl = Info_SubValidate(value);
	if (vl >= MAX_QPATH)
		return false;

	s = Info_RemoveKey(s, key);
	if (!vl)
		return true;

	size_t l = strlen(s);
	if (l + kl + vl + 2 >= MAX_INFO_STRING)
		return false;
	
	s = strconcat(s, "\\", key, "\\", value);
	return true;
}