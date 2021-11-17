#pragma once

#include "config.h"
#include "game/entity_types.h"
#include "lib/math/random.h"
#include "game/util.h"
#include "game/spawn.h"

using fire_func = void(*)(entity &);

inline vector P_ProjectSource(entity &ent, vector point, vector distance, vector forward, vector right, vector up = MOVEDIR_UP)
{
	constexpr float handedness_scales[] = { 1, -1, 0 };
	vector scaled_dist = { distance[0], distance[1] * handedness_scales[ent.client.pers.hand], distance[2] };
	return G_ProjectSource(point, scaled_dist, forward, right, up);
}

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

// always returns false/never matches any frame
constexpr bool G_FrameIsNone(int32_t frame [[maybe_unused]])
{
	return false;
}

// matches any of the specified input frames
template<int32_t ...frames>
constexpr bool G_FrameIsOneOf(int32_t frame)
{
	static_assert(sizeof...(frames) > 0, "Must specify at least one argument");

	return ((frames == frame) || ...);
}

// matches if >= start && <= end
template<int32_t start, int32_t end>
constexpr bool G_FrameIsBetween(int32_t frame)
{
	return frame >= start && frame <= end;
}

using G_WeaponFrameFunc = bool(*) (int32_t frame);

/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
void Weapon_Generic(entity &ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_DEACTIVATE_LAST, G_WeaponFrameFunc is_pause_frame, G_WeaponFrameFunc is_fire_frame, fire_func fire);

/*
================
Throw_Generic

A generic function to handle the basics of throwable thinking
================
*/
void Throw_Generic(entity &ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_THROW_SOUND, stringlit throw_sound, int32_t FRAME_THROW_HOLD, stringlit weapon_sound, int32_t FRAME_THROW_FIRE, G_WeaponFrameFunc is_pause_frame, void (*fire)(entity &, bool), bool explode_in_hand);
