#pragma once

import "lib/types.h";
import "lib/protocol.h";
import "game/entity_types.h";
import "lib/math/vector.h";

void Blaster_Fire(entity &ent, vector g_offset, int32_t damage, bool hyper, entity_effects effect);

void Weapon_Blaster(entity &ent);