#pragma once

#include "lib/types.h"
#include "entity_types.h"
#include "savables.h"

#ifdef SINGLE_PLAYER
void monster_fire_bullet(entity &self, vector start, vector dir, int damage, int kick, int hspread, int vspread, monster_muzzleflash flashtype);
void monster_fire_shotgun(entity &self, vector start, vector aimdir, int damage, int kick, int hspread, int vspread, int count, monster_muzzleflash flashtype);
void monster_fire_blaster(entity &self, vector start, vector dir, int damage, int speed, monster_muzzleflash flashtype, entity_effects effect);
void monster_fire_grenade(entity &self, vector start, vector aimdir, int damage, int speed, monster_muzzleflash flashtype);
void monster_fire_rocket(entity &self, vector start, vector dir, int damage, int speed, monster_muzzleflash flashtype);
void monster_fire_railgun(entity &self, vector start, vector aimdir, int damage, int kick, monster_muzzleflash flashtype);
void monster_fire_bfg(entity &self, vector start, vector aimdir, int damage, int speed, float damage_radius, monster_muzzleflash flashtype);

void M_FliesOff(entity &self);

DECLARE_SAVABLE(M_FliesOff);

void M_FliesOn(entity &self);

DECLARE_SAVABLE(M_FliesOn);

void M_FlyCheck(entity &self);

void M_CheckGround(entity &ent);

void M_CatagorizePosition(entity &ent);

void M_WorldEffects(entity &ent);

void M_droptofloor(entity &ent);

DECLARE_SAVABLE(M_droptofloor);

void M_SetEffects(entity &ent);

/*
==============
M_ReactToDamage

The default handler for monsters reacting to getting hit and changing
their targets appropriately.
==============
*/
void M_ReactToDamage(entity &targ, entity &attacker, entity &inflictor, int32_t knockback, int32_t take);

// default pain implementation, which just toggles the
// pain skin depending on our current health. this assumes that
// every odd numbered skin is a pain skin.
void monster_pain(entity &self, entity &attacker, float knockback, int32_t take);

DECLARE_SAVABLE(monster_pain);

void monster_think(entity &self);

DECLARE_SAVABLE(monster_think);

/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use(entity &self, entity &other, entity &cactivator);

DECLARE_SAVABLE(monster_use);

/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use(entity &self);

void walkmonster_start(entity &self);

void flymonster_start(entity &self);

void swimmonster_start(entity &self);

#ifdef GROUND_ZERO
void stationarymonster_start(entity &self);
#endif

#ifdef ROGUE_AI
void cleanupHealTarget(entity &ent);
#endif

#endif