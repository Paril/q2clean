#pragma once
#include "../lib/types.h"
#include "../lib/entity_effects.h"
#include "combat.h"

/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, means_of_death mod);

/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
*/
void fire_shotgun(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, int32_t count, means_of_death mod);

void fire_blaster(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity_effects effect, means_of_death mod, bool hyper);

void Grenade_Explode(entity &ent);

void fire_grenade(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, float timer, float damage_radius);

void fire_grenade2(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, float timer, float damage_radius, bool held);

entity &fire_rocket(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius, int radius_damage);

void fire_rail(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick);

void fire_bfg(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius);
