#pragma once

#include <exception>

#include "lib/types.h"
#include "lib/types/enum.h"
#include "lib/string.h"

enum spawn_flag : uint32_t
{
	NO_SPAWNFLAGS = 0
};

MAKE_ENUM_BITWISE(spawn_flag);

struct client;
struct entity;

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
	entity *_ptr;

public:
	// initialize an empty reference
	constexpr entityref() :
		_ptr(nullptr)
	{
	}

	// initialize an empty reference
	constexpr entityref(nullptr_t) :
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

	constexpr entity &or_default(entity &def) const
	{
		return has_value() ? *_ptr : def;
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
	constexpr operator entity &()
	{
		if (!has_value())
			throw invalid_entityref();
		return *_ptr;
	}

	// can be converted to an actual entity& implicitly
	constexpr operator entity &() const
	{
		if (!has_value())
			throw invalid_entityref();
		return *_ptr;
	}

	// can be converted to an actual entity& implicitly
	constexpr operator const entity &() const
	{
		if (!has_value())
			throw invalid_entityref();
		return *_ptr;
	}

	// dereference
	constexpr entity &operator*()
	{
		if (!has_value())
			throw invalid_entityref();
		return *_ptr;
	}

	constexpr entity &operator*() const
	{
		if (!has_value())
			throw invalid_entityref();
		return *_ptr;
	}

	// can be converted to an entity pointer implicitly
	constexpr operator entity *() const
	{
		return _ptr;
	}

	// can be converted to an entity pointer implicitly
	constexpr operator const entity *() const
	{
		return _ptr;
	}

	constexpr bool is_world() const;

	// this is explicitly marked as deleted to aid in QC porting.
	// in QC, the bool operand on an entity returns false if the
	// entity is "world". Because Quake II has the concept of a
	// "null" entity, this may be ambiguous and cause issues during
	// porting. you will have to be explicit as to what you want to
	// to: if you wish to see if *any* entity is being held, use e.has_value().
	// if you wish to mimic the behavior of QC, use e.is_world().
	constexpr explicit operator bool() = delete;
};

constexpr entityref null_entity;

// special type to denote entities and register types that need run-time checking.
// most entities don't need a custom ID. Entities can also "inherit" a parent type,
// which allows for a very loose inheritence structure for comparison (for instance,
// func_plat2 can inherit func_plat, which means that those .type comparisons to ET_FUNC_PLAT
// will match both of them).
struct entity_type
{
private:
	static void register_et(entity_type *type);

	entity_type(const entity_type &) = delete;
	entity_type(entity_type &&) = delete;

public:
	stringlit const id;
	const struct registered_entity *const spawn = nullptr;
	const entity_type *next = nullptr;
	const entity_type *parent = nullptr;

	constexpr entity_type() : id("temp")
	{
	}

	constexpr entity_type(stringlit id) : id(id)
	{
		register_et(this);
	}

	constexpr entity_type(stringlit id, const entity_type &parent) :
		id(id),
		parent(&parent)
	{
		register_et(this);
	}

	constexpr bool operator==(const entity_type &rhs) const
	{
		// check whole inheritence chain
		for (const entity_type *e = this; e; e = e->parent)
			if (e == &rhs)
				return true;
		return false;
	}

	constexpr bool operator!=(const entity_type &rhs) const
	{
		return !(*this == rhs);
	}

	constexpr bool is_exact(const entity_type &rhs) const
	{
		return this == &rhs;
	}
};

// special entity type that maps to an unknown entity.
// it's also used as the head of the entity type list.
extern entity_type ET_UNKNOWN;

// simple wrapper to an entity_type-or-unknown
struct entity_type_ref
{
	const entity_type *type;

	constexpr entity_type_ref() : type(&ET_UNKNOWN) { }
	constexpr entity_type_ref(const entity_type &type) : type(&type) { }

	constexpr const entity_type *operator->() const { return type; }

	constexpr operator bool() const { return type != &ET_UNKNOWN; }

	constexpr bool operator==(const entity_type_ref &rhs) const { return *type == *rhs.type; }
	constexpr bool operator!=(const entity_type_ref &rhs) const { return *type != *rhs.type; }

	constexpr bool operator==(const entity_type &rhs) const { return *type == rhs; }
	constexpr bool operator!=(const entity_type &rhs) const { return *type != rhs; }

	constexpr operator const entity_type &() const { return *type; }
};