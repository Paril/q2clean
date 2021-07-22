#pragma once

#include "config.h"
#include "game/entity_types.h"
#include "itemlist.h"

bool Add_Ammo(entity &ent, const gitem_t &it, int32_t count);

bool Pickup_Ammo(entity &ent, entity &other);

void Drop_Ammo(entity &ent, const gitem_t &it);

bool Pickup_Weapon(entity &ent, entity &other);

/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(entity &ent, const gitem_t &it);

void Drop_Weapon(entity &ent, const gitem_t &it);