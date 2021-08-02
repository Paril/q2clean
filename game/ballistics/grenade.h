#pragma once

#include "config.h"
#include "game/entity_types.h"
#include "lib/math/vector.h"
#include "game/spawn.h"

void Grenade_Explode(entity &ent);

void fire_grenade(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, float timer, float damage_radius);

void fire_grenade2(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, float timer, float damage_radius, bool held);

DECLARE_ENTITY(GRENADE);
