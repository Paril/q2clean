#pragma once

#include "types.h"

// thrown on an invalid entity dereference
class invalid_entityref : public std::exception
{
public:
	invalid_entityref() :
		std::exception("attempted to dereference a null entityref", 1)
	{
	}
};

// entityref is a simple type that holds an optional reference to an entity, but only
// takes up the space of a pointer. the pointer cannot be swapped without assignment.
// they are always initialized as zeroes.
struct entityref
{
private:
	entity	*_ptr;

public:
	// initialize an empty reference
	constexpr entityref() :
		_ptr(nullptr)
	{
	}

	// initialize an empty reference
	constexpr entityref(std::nullptr_t) :
		_ptr(nullptr)
	{
	}
	
	// initialize a reference to a specific entity
	constexpr entityref(entity &ref) :
		_ptr(&ref)
	{
	}

	// initialize a reference to a specific entity
	constexpr entityref(const entity &ref) :
		_ptr(const_cast<entity *>(&ref))
	{
	}

	// check whether this entityref is holding an entity
	// reference or not
	constexpr bool has_value() const
	{
		return !!_ptr;
	}

	// check if this entityref holds the same reference as another entityref
	constexpr bool operator==(const entityref &rhs) const { return _ptr == rhs._ptr; }

	// check if this entityref doesn't hold the same reference as another entityref
	constexpr bool operator!=(const entityref &rhs) const { return _ptr != rhs._ptr; }

	// check if this entityref holds this entity
	constexpr bool operator==(const entity &rhs) const { return _ptr == &rhs; }

	// check if this entityref doesn't hold this entity
	constexpr bool operator!=(const entity &rhs) const { return _ptr != &rhs; }

	// you can use -> to easily dereference an entity reference
	// for quick access checks before copying it out. this will
	// throw if the entity is invalid.
	inline entity *operator->()
	{
		if (!has_value())
			throw invalid_entityref();
		return _ptr;
	}

	// you can use -> to easily dereference an entity reference
	// for quick access checks before copying it out. this will
	// throw if the entity is invalid.
	inline entity *operator->() const
	{
		if (!has_value())
			throw invalid_entityref();
		return _ptr;
	}

	// can be converted to an actual entity& implicitly
	constexpr operator entity&() const
	{
		if (!has_value())
			throw invalid_entityref();
		return *_ptr;
	}

	// can be converted to an actual entity& implicitly
	constexpr operator const entity&() const
	{
		if (!has_value())
			throw invalid_entityref();
		return *_ptr;
	}

	// can be converted to an entity pointer implicitly
	constexpr operator entity*() const
	{
		return _ptr;
	}

	// can be converted to an entity pointer implicitly
	constexpr operator const entity*() const
	{
		return _ptr;
	}

	// this is explicitly marked as deleted to aid in QC porting.
	// in QC, the bool operand on an entity returns false if the
	// entity is "world". Because Quake II has the concept of a
	// "null" entity, this may be ambiguous and cause issues during
	// porting. you will have to be explicit as to what you want to
	// to: if you wish to see if *any* entity is being held, use e.has_value().
	// if you wish to mimic the behavior of QC, use e.has_value() && e->is_world().
	constexpr operator bool() = delete;
};

constexpr entityref null_entity;