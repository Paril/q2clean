#pragma once

#include "../lib/types.h"
#include "../lib/gi.h"
#include "../lib/entity.h"
#include "util.h"

extern bool		is_quad;
extern muzzleflash	is_silenced;

extern int32_t	damage_multiplier;

using fire_func = void(entity &);

void P_DamageModifier(entity &ent);

inline vector P_ProjectSource(entity &ent, vector point, vector distance, vector forward, vector right)
{
	constexpr float handedness_scales[] = { 1, -1, 0 };
	vector scaled_dist = { distance[0], distance[1] * handedness_scales[ent.client->g.pers.hand], distance[2] };
	return G_ProjectSource(point, scaled_dist, forward, right);
}

bool Pickup_Weapon(entity &ent, entity &other);

/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon(entity &ent);

void NoAmmoWeaponChange(entity &ent);

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon(entity &ent);

void Use_Weapon(entity &ent, const gitem_t &it);

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(entity &ent, const gitem_t &it);

void Weapon_Generic(entity &ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_DEACTIVATE_LAST, std::initializer_list<int32_t> is_pause_frame, std::initializer_list<int32_t> is_fire_frame, fire_func *fire);

constexpr float GRENADE_TIMER	= 3.0f;
constexpr int GRENADE_MINSPEED	= 400;
constexpr int GRENADE_MAXSPEED	= 800;

void Weapon_Grenade(entity &ent);
void Weapon_GrenadeLauncher(entity &ent);
void Weapon_RocketLauncher(entity &ent);
void Weapon_Blaster(entity &ent);
void Weapon_HyperBlaster(entity &ent);
void Weapon_Machinegun(entity &ent);
void Weapon_Chaingun(entity &ent);
void Weapon_Shotgun(entity &ent);
void Weapon_SuperShotgun(entity &ent);
void Weapon_Railgun(entity &ent);
void Weapon_BFG(entity &ent);

#ifdef SINGLE_PLAYER
void PlayerNoise(entity &who, vector where, player_noise_type type)
#endif