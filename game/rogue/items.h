#pragma once

#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity_types.h"
#include "game/items/itemlist.h"

void Use_Nuke(entity &ent, const gitem_t &it);

bool Pickup_Nuke(entity &ent, entity &other);

void Use_Double(entity &ent, const gitem_t &it);

void Use_IR(entity &ent, const gitem_t &it);

void SetTriggeredSpawn(entity &ent);
#endif