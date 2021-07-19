module;

#include "lib/types.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/gweapon.h"

export module weaponry.rocketlauncher;

import weaponry;
import player.view;
import math.vector;
import math.random;

static void Weapon_RocketLauncher_Fire(entity &ent)
{
	vector	offset, start;
	vector	forward, right;
	int32_t	damage;
	float	damage_radius;
	int32_t	radius_damage;

	damage = (int32_t) random(100.f, 120.f);
	radius_damage = 120;
	damage_radius = 120.f;
	if (is_quad) {
		damage *= damage_multiplier;
		radius_damage *= damage_multiplier;
	}

	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);

	ent.client->kick_origin = forward * -2;
	ent.client->kick_angles[0] = -1.f;

	offset = { 8, 8, ent.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);
	fire_rocket(ent, start, forward, damage, 650, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t) ent.s.number);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags) dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;
}

export void Weapon_RocketLauncher(entity &ent)
{
	Weapon_Generic(ent, 4, 12, 50, 54, { 25, 33, 42, 50 }, { 5 }, Weapon_RocketLauncher_Fire);
}