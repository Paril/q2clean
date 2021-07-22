#pragma once

#include "lib/types.h"
#include "lib/protocol.h"
#include "game/entity_types.h"
#include "lib/math/vector.h"

void Blaster_Fire(entity &ent, vector g_offset, int32_t damage, bool hyper, entity_effects effect);

void Weapon_Blaster(entity &ent);