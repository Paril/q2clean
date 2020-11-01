#pragma once

#include "types.h"

enum cvar_flags : int32_t
{
	CVAR_NONE,
	// set to cause it to be saved to config.cfg
	CVAR_ARCHIVE	= 1 << 0,
	// added to userinfo  when changed
	CVAR_USERINFO	= 1 << 1,
	// added to serverinfo when changed
	CVAR_SERVERINFO	= 1 << 2,
	// don't allow change from console at all,
	// but can be set from the command line
	CVAR_NOSET		= 1 << 3,
	// save changes until server restart
	CVAR_LATCH		= 1 << 4
};

MAKE_ENUM_BITWISE(cvar_flags);

// console variables
struct cvar
{
	cvar() = delete;

	stringlit	name;
	stringlit	string;
	// for CVAR_LATCH vars, this is the current value
	stringlit	latched_string;
	const cvar_flags	flags;
	// set each time the cvar is changed
	qboolean			modified;
	const float			value;
};

// a wrapper for cvar that can perform conversions automatically.
struct cvarref
{
private:
	cvar *cv;
	static constexpr cvar_flags empty_flags = CVAR_NONE; 
	static qboolean empty_modified; 
	static constexpr float empty_value = 0.f; 

public:
	stringlit			name;
	stringlit			string;
	stringlit			latched_string;
	const cvar_flags	&flags;
	qboolean			&modified;
	const float			&value;

	cvarref() :
		cv(nullptr),
		name(nullptr),
		string(nullptr),
		latched_string(nullptr),
		flags(empty_flags),
		modified(empty_modified),
		value(empty_value)
	{
	}

	cvarref(nullptr_t) :
		cvarref()
	{
	}

	cvarref(cvar &v) :
		cv(&v),
		name(v.name),
		string(v.string),
		latched_string(v.latched_string),
		flags(v.flags),
		modified(v.modified),
		value(v.value)
	{
	}
	
	cvarref &operator=(const cvarref &tr)
	{
		new(this) cvarref(tr);
		return *this;
	}

	// string literal conversion
	inline explicit operator stringlit() const
	{
		return string;
	}

	// bool conversion catches empty strings and "0" as false, everything else
	// as true
	inline explicit operator bool() const
	{
		return *string && !(string[0] == '0' && string[1] == '\0');
	}

	// everything else is templated
	template<typename T>
	inline explicit operator T() const
	{
		return (T)value;
	}

	inline bool operator==(stringlit lit) const { return (stringref)string == lit; }
	inline bool operator!=(stringlit lit) const { return (stringref)string != lit; }

	inline bool operator==(const stringref &ref) const { return ref == string; }
	inline bool operator!=(const stringref &ref) const { return ref != string; }
};