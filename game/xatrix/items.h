#pragma once

#include "config.h"

#ifdef THE_RECKONING
#include "game/entity_types.h"
#include "game/game_types.h"
#include "game/spawn.h"
#include "game/items/itemlist.h"

extern gtime quad_fire_drop_timeout_hack;

void Use_QuadFire(entity &ent, const gitem_t &it);
#endif