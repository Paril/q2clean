#pragma once

import <exception>;
import "config.h";
import "lib/string.h";
import "lib/protocol.h";
import "lib/types/array.h";
import "lib/types/enum.h";
import "game/entity_types.h";

// gitem_t.flags
enum gitem_flags : uint16_t
{
	IT_WEAPON = 1 << 0,       // use makes active weapon
	IT_AMMO = 1 << 1,
	IT_ARMOR = 1 << 2,
	IT_HEALTH = 1 << 3,
#ifdef SINGLE_PLAYER
	IT_STAY_COOP = 1 << 4,
	IT_KEY = 1 << 5,
#endif
#ifdef CTF
	IT_TECH = 1 << 6,
	IT_FLAG = 1 << 7,
#endif
	IT_POWERUP = 1 << 8,

#ifdef SINGLE_PLAYER
	IT_WEAPON_MASK = IT_WEAPON | IT_STAY_COOP
#else
	IT_WEAPON_MASK = IT_WEAPON
#endif
};

MAKE_ENUM_BITWISE(gitem_flags);

enum ammo_id : uint8_t
{
	AMMO_BULLETS,
	AMMO_SHELLS,
	AMMO_ROCKETS,
	AMMO_GRENADES,
	AMMO_CELLS,
	AMMO_SLUGS,

#ifdef THE_RECKONING
	AMMO_MAGSLUG,
	AMMO_TRAP,
#endif

#ifdef GROUND_ZERO
	AMMO_FLECHETTES,
	AMMO_TESLA,
	AMMO_PROX,
	AMMO_DISRUPTOR,
#endif

	AMMO_TOTAL
};

#define MARKER_START(n) \
	n, \
	marker_##n = n - 1

#define MARKER_END(n) \
	marker_##n, \
	n = marker_##n - 1

// you have a max of MAX_ITEMS items to list here
enum gitem_id : uint8_t
{
	ITEM_NONE,

	ITEM_ARMOR_BODY,
	ITEM_ARMOR_COMBAT,
	ITEM_ARMOR_JACKET,
	ITEM_ARMOR_SHARD,

	ITEM_POWER_SCREEN,
	ITEM_POWER_SHIELD,

	MARKER_START(ITEM_WEAPONS_FIRST),

#ifdef GRAPPLE
	ITEM_GRAPPLE,
#endif
	ITEM_BLASTER,
#ifdef GROUND_ZERO
	ITEM_CHAINFIST,
#endif
	ITEM_SHOTGUN,
	ITEM_SUPER_SHOTGUN,
	ITEM_MACHINEGUN,
#ifdef GROUND_ZERO
	ITEM_ETF_RIFLE,
#endif
	ITEM_CHAINGUN,
	ITEM_GRENADES,
#ifdef THE_RECKONING
	ITEM_TRAP,
#endif
#ifdef GROUND_ZERO
	ITEM_TESLA,
#endif
	ITEM_GRENADE_LAUNCHER,
#ifdef GROUND_ZERO
	ITEM_PROX_LAUNCHER,
#endif
	ITEM_ROCKET_LAUNCHER,
	ITEM_HYPERBLASTER,
#ifdef THE_RECKONING
	ITEM_BOOMER,
#endif
#ifdef GROUND_ZERO
	ITEM_PLASMA_BEAM,
#endif
	ITEM_RAILGUN,
#ifdef THE_RECKONING
	ITEM_PHALANX,
#endif
	ITEM_BFG,
#ifdef GROUND_ZERO
	ITEM_DISINTEGRATOR,
#endif

	MARKER_END(ITEM_WEAPONS_LAST),

	ITEM_SHELLS,
	ITEM_BULLETS,
	ITEM_CELLS,
	ITEM_ROCKETS,
	ITEM_SLUGS,
#ifdef THE_RECKONING
	ITEM_MAGSLUG,
#endif
#ifdef GROUND_ZERO
	ITEM_FLECHETTES,
	ITEM_PROX,
	ITEM_NUKE,
	ITEM_ROUNDS,
#endif

	ITEM_QUAD_DAMAGE,
	ITEM_INVULNERABILITY,
	ITEM_SILENCER,
	ITEM_BREATHER,
	ITEM_ENVIRO,
#ifdef THE_RECKONING
	ITEM_QUADFIRE,
#endif
#ifdef GROUND_ZERO
	ITEM_IR_GOGGLES,
	ITEM_DOUBLE_DAMAGE,
#endif
	ITEM_ANCIENT_HEAD,
	ITEM_ADRENALINE,
	ITEM_BANDOLIER,
	ITEM_PACK,

#ifdef SINGLE_PLAYER
	ITEM_DATA_CD,
	ITEM_POWER_CUBE,
	ITEM_PYRAMID_KEY,
	ITEM_DATA_SPINNER,
	ITEM_SECURITY_PASS,
	ITEM_BLUE_KEY,
	ITEM_RED_KEY,
	ITEM_COMMANDER_HEAD,
	ITEM_AIRSTRIKE_TARGET,
#ifdef THE_RECKONING
	ITEM_GREEN_KEY,
#endif
#ifdef GROUND_ZERO
	ITEM_NUKE_CONTAINER,
	ITEM_ANTIMATTER_BOMB,
#endif
#endif

#ifdef CTF
	ITEM_FLAG1,
	ITEM_FLAG2,
	ITEM_TECH_1,
	ITEM_TECH_2,
	ITEM_TECH_3,
	ITEM_TECH_4,
#endif

	ITEM_HEALTH,

	ITEM_TOTAL
};

struct gitem_t;

using pickup_func = bool(entity &, entity &);
using use_func = void(entity &, const gitem_t &);
using drop_func = use_func;
using weaponthink_func = void(entity &);

struct gitem_armor
{
	int32_t	base_count;
	int32_t	max_count;

	float	normal_protection;
	float	energy_protection;
};

struct gitem_t
{
	stringlit			classname; // spawning name
	pickup_func			*pickup;
	use_func			*use;
	drop_func			*drop;
	weaponthink_func	*weaponthink;
	stringlit			pickup_sound;
	stringlit			world_model;
	entity_effects		world_model_flags;
	stringlit			view_model;
	stringlit			vwep_model;
	
	// client side info
	stringlit	icon;
	stringlit	pickup_name;	// for printing on pickup
	
	uint16_t	quantity;	// for ammo how much, for weapons how much is used per shot
	gitem_id	ammo;		// for weapons
	gitem_flags	flags;		// IT_* flags

	gitem_armor	armor;
	ammo_id		ammotag;
	
	stringlit	precaches;	// string of all models, sounds, and images this item will use

	// do NOT set these via the itemlist; they are overwritten later!

	// the item's ID
	gitem_id	id;
	// weapon model index (for weapons)
	uint8_t		vwep_id;
};

// thrown on an invalid item dereference
class invalid_itemref : public std::exception
{
public:
	invalid_itemref() :
		std::exception("attempted to dereference a null item", 1)
	{
	}
};

// itemref holds a reference to a valid item, or nullptr.
struct itemref
{
private:
	const gitem_t *_ptr;

public:
	// initialize an empty reference
	itemref();

	// initialize an empty reference
	inline itemref(std::nullptr_t) :
		itemref()
	{
	}

	// initialize a reference to a specific item
	inline itemref(const gitem_t &ref) :
		_ptr(&ref)
	{
	}

	// initialize a reference to a specific item
	itemref(const gitem_id &ref);

	// check whether this itemref is holding an item
	// reference or not
	bool has_value() const;

	// check if this itemref holds the same reference as another itemref
	inline bool operator==(const itemref &rhs) const { return _ptr == rhs._ptr; }

	// check if this itemref doesn't hold the same reference as another itemref
	inline bool operator!=(const itemref &rhs) const { return _ptr != rhs._ptr; }

	// check if this itemref holds this item
	inline bool operator==(const gitem_t &rhs) const { return _ptr == &rhs; }

	// check if this itemref doesn't hold this item
	inline bool operator!=(const gitem_t &rhs) const { return _ptr != &rhs; }

	// you can use -> to easily dereference an item reference
	// for quick access checks before copying it out. this won't
	// throw if the item is invalid, but just returns the null item's
	// info.
	inline const gitem_t *operator->()
	{
		return _ptr;
	}

	// you can use -> to easily dereference an item reference
	// for quick access checks before copying it out. this won't
	// throw if the item is invalid, but just returns the null item's
	// info.
	inline const gitem_t *operator->() const
	{
		return _ptr;
	}

	// can be converted to an actual item& implicitly
	inline operator const gitem_t &() const
	{
		return *_ptr;
	}

	// can be converted to an entity pointer implicitly
	inline operator const gitem_t *() const
	{
		return _ptr;
	}

	inline explicit operator bool() const
	{
		return has_value();
	}
};

// fetch the immutable item list
const array<gitem_t, ITEM_TOTAL> &item_list();

// fetch specific item by id, with bounds check
inline const gitem_t &GetItemByIndex(const gitem_id &id)
{
	return item_list()[id];
}

/*
===============
FindItemByClassname
===============
*/
inline itemref FindItemByClassname(const stringref &classname)
{
	for (const gitem_t &item : item_list())
		if (striequals(item.classname, classname))
			return item;

	return nullptr;
}

/*
===============
FindItem
===============
*/
inline itemref FindItem(const stringref &pickup_name)
{
	for (const gitem_t &item : item_list())
		if (striequals(item.pickup_name, pickup_name))
			return item;

	return nullptr;
}

void InitItems();