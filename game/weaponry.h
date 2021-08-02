#pragma once

#include "config.h"
#include "game/entity_types.h"
#include "lib/math/random.h"
#include "game/util.h"
#include "game/spawn.h"

using fire_func = void(entity &);

inline vector P_ProjectSource(entity &ent, vector point, vector distance, vector forward, vector right, vector up = MOVEDIR_UP)
{
	constexpr float handedness_scales[] = { 1, -1, 0 };
	vector scaled_dist = { distance[0], distance[1] * handedness_scales[ent.client->pers.hand], distance[2] };
	return G_ProjectSource(point, scaled_dist, forward, right, up);
}

extern bool			is_quad;
extern muzzleflash	is_silenced;

extern int32_t	damage_multiplier;

void P_DamageModifier(entity &ent);

#ifdef SINGLE_PLAYER
DECLARE_ENTITY(PLAYER_NOISE);

enum player_noise : int32_t
{
	PNOISE_SELF,
	PNOISE_WEAPON,
	PNOISE_IMPACT
};

/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(entity &who, vector where, player_noise type);
#endif

void ChangeWeapon(entity &ent);

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(entity &ent);

void Think_Weapon(entity &ent);

constexpr bool G_IsAnyFrame(int32_t frame [[maybe_unused]])
{
	return false;
}

template<int32_t ...frames>
constexpr bool G_IsAnyFrame(int32_t frame)
{
	return ((frames == frame) || ...);
}

using G_WeaponFrameFunc = bool(*) (int32_t frame);

/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
void Weapon_Generic(entity &ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_DEACTIVATE_LAST, G_WeaponFrameFunc is_pause_frame, G_WeaponFrameFunc is_fire_frame, fire_func *fire);