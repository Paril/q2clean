#pragma once

import <exception>;

import "lib/types.h";
import "lib/types/enum.h";

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

	// this is explicitly marked as deleted to aid in QC porting.
	// in QC, the bool operand on an entity returns false if the
	// entity is "world". Because Quake II has the concept of a
	// "null" entity, this may be ambiguous and cause issues during
	// porting. you will have to be explicit as to what you want to
	// to: if you wish to see if *any* entity is being held, use e.has_value().
	// if you wish to mimic the behavior of QC, use e.has_value() && e->is_world().
	constexpr explicit operator bool() = delete;
};

constexpr entityref null_entity;

// entity type; this is to replace classname comparisons. Every entity spawnable by
// classnames must have their own ID. You can assign IDs to other entities if you're
// wanting to search for them, or anything like that.
enum entity_type : uint32_t
{
	// entity doesn't have a type assigned
	ET_UNKNOWN,

	// freed entity
	ET_FREED,

	// items spawned in the world
	ET_ITEM,

	// special values for run-time entities
	ET_BODYQUEUE,
	ET_BLASTER_BOLT,
	ET_GRENADE,
	ET_HANDGRENADE,
	ET_ROCKET,
	ET_BFG_BLAST,
	ET_DEBRIS,
	ET_PLAYER,
	ET_DISCONNECTED_PLAYER,
	ET_DELAYED_USE,

	// spawnable entities
	ET_FUNC_PLAT,
	ET_FUNC_ROTATING,
	ET_FUNC_BUTTON,
	ET_FUNC_DOOR,
	ET_FUNC_DOOR_ROTATING,
	ET_FUNC_KILLBOX,
	ET_FUNC_TRAIN,
	ET_TRIGGER_ELEVATOR,
	ET_FUNC_TIMER,
	ET_FUNC_CONVEYOR,
	ET_FUNC_GROUP,
	ET_FUNC_AREAPORTAL,
	ET_PATH_CORNER,
	ET_INFO_NOTNULL,
	ET_LIGHT,
	ET_FUNC_WALL,
	ET_FUNC_OBJECT,
	ET_MISC_BLACKHOLE,
	ET_MISC_EASTERTANK,
	ET_MISC_EASTERCHICK,
	ET_MISC_EASTERCHICK2,
	ET_MONSTER_COMMANDER_BODY,
	ET_MISC_BANNER,
	ET_MISC_VIPER,
	ET_MISC_BIGVIPER,
	ET_MISC_VIPER_BOMB,
	ET_MISC_STROGG_SHIP,
	ET_MISC_SATELLITE_DISH,
	ET_LIGHT_MINE1,
	ET_LIGHT_MINE2,
	ET_MISC_GIB_ARM,
	ET_MISC_GIB_LEG,
	ET_MISC_GIB_HEAD,
	ET_TARGET_CHARACTER,
	ET_TARGET_STRING,
	ET_FUNC_CLOCK,
	ET_MISC_TELEPORTER,
	ET_MISC_TELEPORTER_DEST,
	ET_INFO_PLAYER_START,
	ET_INFO_PLAYER_DEATHMATCH,
	ET_INFO_PLAYER_INTERMISSION,
	ET_WORLDSPAWN,
	ET_TARGET_TEMP_ENTITY,
	ET_TARGET_SPEAKER,
	ET_TARGET_EXPLOSION,
	ET_TARGET_CHANGELEVEL,
	ET_TARGET_SPLASH,
	ET_TARGET_SPAWNER,
	ET_TARGET_BLASTER,
	ET_TARGET_LASER,
	ET_TARGET_EARTHQUAKE,
	ET_TRIGGER_MULTIPLE,
	ET_TRIGGER_ONCE,
	ET_TRIGGER_RELAY,
	ET_TRIGGER_COUNTER,
	ET_TRIGGER_ALWAYS,
	ET_TRIGGER_PUSH,
	ET_TRIGGER_HURT,
	ET_TRIGGER_GRAVITY,

#ifdef SINGLE_PLAYER
	ET_INFO_PLAYER_COOP,

	ET_TARGET_HELP,
	ET_TARGET_SECRET,
	ET_TARGET_GOAL,
	ET_TARGET_CROSSLEVEL_TRIGGER,
	ET_TARGET_CROSSLEVEL_TARGET,
	ET_TARGET_LIGHTRAMP,
	ET_TARGET_ACTOR,

	ET_TRIGGER_KEY,
	ET_TRIGGER_MONSTERJUMP,

	ET_PLAYER_NOISE,
	ET_PLAYER_TRAIL,

	ET_POINT_COMBAT,
	ET_FUNC_EXPLOSIVE,
	ET_MISC_EXPLOBOX,
	ET_MISC_DEADSOLDIER,

	ET_MONSTER_SOLDIER_LIGHT,
	ET_MONSTER_SOLDIER,
	ET_MONSTER_SOLDIER_SS,
	ET_MONSTER_MEDIC,
	ET_MONSTER_TANK,
	ET_MONSTER_SUPERTANK,
	ET_MONSTER_MAKRON,
	ET_MONSTER_JORG,
#endif

	// special index for all entity types. save games use this
	// to check for compatibility.
	ET_TOTAL
};
