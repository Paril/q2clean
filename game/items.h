#pragma once

#include "../lib/types.h"
#include "spawn_flag.h"
#include "../lib/savables.h"

import protocol;
import math.vector;
import items.list;

// item spawnflags
constexpr spawn_flag ITEM_TRIGGER_SPAWN		= (spawn_flag)0x00000001;
constexpr spawn_flag ITEM_NO_TOUCH			= (spawn_flag)0x00000002;
// 6 bits reserved for editor flags
// 8 bits used as power cube id bits for coop games
constexpr spawn_flag DROPPED_ITEM			= (spawn_flag)0x00010000;
constexpr spawn_flag DROPPED_PLAYER_ITEM	= (spawn_flag)0x00020000;
constexpr spawn_flag ITEM_TARGETS_USED		= (spawn_flag)0x00040000;

void DoRespawn(entity &ent);

DECLARE_SAVABLE_FUNCTION(DoRespawn);

entity &Drop_Item(entity &, const gitem_t &);

void SetRespawn(entity &ent, float delay);

void Touch_Item(entity &ent, entity &other, vector plane, const surface &surf);

DECLARE_SAVABLE_FUNCTION(Touch_Item);

void droptofloor(entity &ent);

DECLARE_SAVABLE_FUNCTION(droptofloor);

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
