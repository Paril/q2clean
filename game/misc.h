#pragma once

#include "config.h"
#include "lib/types.h"
#include "lib/string.h"
#include "lib/protocol.h"
#include "lib/math/random.h"
#include "game/entity_types.h"
#include "game/spawn.h"

inline vector VelocityForDamage(int32_t damage)
{
	return randomv({ -100, -100, 200 }, { 100, 100, 300 }) * ((damage < 50) ? 0.7f : 1.2f);
}

void ClipGibVelocity(entity &ent);

void gib_touch(entity &self, entity &other, vector normal, const surface &surf);

DECLARE_SAVABLE(gib_touch);

void gib_die(entity &self, entity &inflictor, entity &attacker, int32_t damage, vector point);

DECLARE_SAVABLE(gib_die);

// gib types
enum gib_type : uint8_t
{
	GIB_ORGANIC,
	GIB_METALLIC
};

entity &ThrowGib(entity &self, stringlit gibname, int32_t damage, gib_type type);

void ThrowHead(entity &self, stringlit gibname, int32_t damage, gib_type type);

void ThrowClientHead(entity &self, int32_t damage);

void ThrowDebris(entity &self, stringlit modelname, float speed, vector origin);

void misc_viper_use(entity &self, entity &other, entity &cactivator);

DECLARE_SAVABLE(misc_viper_use);

void misc_strogg_ship_use(entity &self, entity &other, entity &cactivator);

DECLARE_SAVABLE(misc_strogg_ship_use);

void SP_misc_teleporter_dest(entity &ent);

DECLARE_ENTITY(FUNC_AREAPORTAL);

DECLARE_ENTITY(MISC_TELEPORTER_DEST);

DECLARE_ENTITY(MISC_EXPLOBOX);

DECLARE_ENTITY(POINT_COMBAT);

DECLARE_ENTITY(PATH_CORNER);

DECLARE_ENTITY(LIGHT);