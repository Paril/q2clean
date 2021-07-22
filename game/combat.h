#pragma once

#include "config.h"
#include "entity_types.h"
#include "game.h"
#include "cmds.h"
#include "lib/types/enum.h"
#include "lib/math/vector.h"
#ifdef SINGLE_PLAYER
#include "monster.h"
#endif

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool CanDamage(entity &targ, entity &inflictor);

// damage flags
enum damage_flags : uint16_t
{
	DAMAGE_NONE,
	DAMAGE_RADIUS = 1 << 0,			// damage was indirect
	DAMAGE_NO_ARMOR = 1 << 1,		// armour does not protect from this damage
	DAMAGE_ENERGY = 1 << 2,			// damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK = 1 << 3,	// do not affect velocity, just view angles
	DAMAGE_BULLET = 1 << 4,			// damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION = 1 << 5	// armor, shields, invulnerability, and godmode have no effect

#ifdef GROUND_ZERO
	, DAMAGE_DESTROY_ARMOR = 1 << 6,// damage is done to armor and health.
	DAMAGE_NO_REG_ARMOR = 1 << 7,	// damage skips regular armor
	DAMAGE_NO_POWER_ARMOR = 1 << 8	// damage skips power armor
#endif
};

// means of death
enum means_of_death : uint8_t
{
	MOD_NONE,

	MOD_UNKNOWN,
	MOD_BLASTER,
	MOD_SHOTGUN,
	MOD_SSHOTGUN,
	MOD_MACHINEGUN,
	MOD_CHAINGUN,
	MOD_GRENADE,
	MOD_G_SPLASH,
	MOD_ROCKET,
	MOD_R_SPLASH,
	MOD_HYPERBLASTER,
	MOD_RAILGUN,
	MOD_BFG_LASER,
	MOD_BFG_BLAST,
	MOD_BFG_EFFECT,
	MOD_HANDGRENADE,
	MOD_HG_SPLASH,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_HELD_GRENADE,
	MOD_EXPLOSIVE,
	MOD_BARREL,
	MOD_BOMB,
	MOD_EXIT,
	MOD_SPLASH,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_HIT,
	MOD_TARGET_BLASTER,

#ifdef THE_RECKONING
	MOD_RIPPER,
	MOD_PHALANX,
	MOD_BRAINTENTACLE,
	MOD_BLASTOFF,
	MOD_GEKK,
	MOD_TRAP,
#endif

#ifdef GROUND_ZERO
	MOD_CHAINFIST,
	MOD_DISINTEGRATOR,
	MOD_ETF_RIFLE,
	MOD_BLASTER2,
	MOD_HEATBEAM,
	MOD_TESLA,
	MOD_PROX,
	MOD_NUKE,
	MOD_TRACKER,
#endif

#ifdef HOOK_CODE
	MOD_GRAPPLE,
#endif

	// bitflag
	MOD_FRIENDLY_FIRE = 1 << 7
};

MAKE_ENUM_BITWISE(means_of_death);

extern means_of_death meansOfDeath;

bool CheckTeamDamage(means_of_death mod [[maybe_unused]]);

/*
============
T_Damage

targ        entity that is being damaged
inflictor   entity that is causing the damage
attacker    entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir         direction of the attack
point       point at which the damage is being inflicted
normal      normal vector from that point
damage      amount of damage being inflicted
knockback   force to be applied against targ as a result of the damage

dflags      these flags are used to control how T_Damage works
mod         the means of death to be displayed on death
============
*/
void T_Damage(entity &targ, entity &inflictor, entity &attacker, vector dir, vector point, vector normal, int32_t damage, int32_t knockback, damage_flags dflags, means_of_death mod);

/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage(entity &inflictor, entity &attacker, float damage, entityref ignore, float radius, means_of_death mod);