#pragma once

#include "../lib/types.h"

inline vector VelocityForDamage(int damage)
{
	return randomv({ -100, -100, 200 }, { 100, 100, 300 }) * ((damage < 50) ? 0.7f : 1.2f);
}

void ClipGibVelocity(entity &ent);

void gib_touch(entity &self, entity &other, vector normal, const surface &surf);

void gib_die(entity &self, entity &inflictor, entity &attacker, int32_t damage, vector point);

// gib types
enum gib_type : uint8_t
{
	GIB_ORGANIC,
	GIB_METALLIC
};

void ThrowGib(entity &self, stringlit gibname, int32_t damage, gib_type type);

void ThrowHead(entity &self, stringlit gibname, int32_t damage, gib_type type);

void ThrowClientHead(entity &self, int32_t damage);

void ThrowDebris(entity &self, stringlit modelname, float speed, vector origin);

void misc_viper_use(entity &self, entity &other, entity &cactivator);

void misc_strogg_ship_use(entity &self, entity &other, entity &cactivator);

void SP_misc_teleporter_dest(entity &ent);
