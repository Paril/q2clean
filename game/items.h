#pragma once

#include "../lib/types.h"
#include "../lib/surface.h"
#include "spawn_flag.h"

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

#define ITEM_COUNT(marker) \
	ITEM_##marker##_LAST - ITEM_##marker##_FIRST + 1

struct gitem_t;

// item spawnflags
constexpr spawn_flag ITEM_TRIGGER_SPAWN		= (spawn_flag)0x00000001;
constexpr spawn_flag ITEM_NO_TOUCH			= (spawn_flag)0x00000002;
// 6 bits reserved for editor flags
// 8 bits used as power cube id bits for coop games
constexpr spawn_flag DROPPED_ITEM			= (spawn_flag)0x00010000;
constexpr spawn_flag DROPPED_PLAYER_ITEM	= (spawn_flag)0x00020000;
constexpr spawn_flag ITEM_TARGETS_USED		= (spawn_flag)0x00040000;

// special values for health styles
constexpr spawn_flag HEALTH_IGNORE_MAX	= (spawn_flag)1;
constexpr spawn_flag HEALTH_TIMED		= (spawn_flag)2;

void InitItems();

void DoRespawn(entity &ent);

entity &Drop_Item(entity &, const gitem_t &);

void SetRespawn(entity &ent, float delay);

bool Add_Ammo(entity &ent, const gitem_t &it, int32_t count);

gitem_id ArmorIndex(entity &ent);

gitem_id PowerArmorType(entity &ent);

void Touch_Item(entity &ent, entity &other, vector plane, const surface &surf);

void droptofloor(entity &ent);

void PrecacheItem(const gitem_t &it);

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem(entity &ent, const gitem_t &it);

/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames();