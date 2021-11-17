#include "config.h"
#include "game/savables.h"
#include "game/entity_types.h"
#include "game/game_types.h"
#include "lib/math/vector.h"
#include "lib/protocol.h"
#include "itemlist.h"

// item spawnflags
constexpr spawn_flag ITEM_TRIGGER_SPAWN = (spawn_flag) (1 << 0);
constexpr spawn_flag ITEM_NO_TOUCH = (spawn_flag) (1 << 1);
// 6 bits reserved for editor flags
constexpr spawn_flag DROPPED_ITEM = (spawn_flag) (1 << 8);
constexpr spawn_flag DROPPED_PLAYER_ITEM = (spawn_flag) (1 << 9);
constexpr spawn_flag ITEM_TARGETS_USED = (spawn_flag) (1 << 10);

void DoRespawn(entity &item);

void SetRespawn(entity &ent, gtimef delay);

void Touch_Item(entity &ent, entity &other, vector plane, const surface &surf);

DECLARE_SAVABLE(Touch_Item);

void droptofloor(entity &ent);

DECLARE_SAVABLE(droptofloor);

entity &Drop_Item(entity &ent, const gitem_t &it);

/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
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

// base type for item entities
extern entity_type ET_ITEM;