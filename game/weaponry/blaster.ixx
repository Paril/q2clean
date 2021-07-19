module;

#include "lib/types.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/gweapon.h"

export module weaponry.blaster;

import weaponry;
import player.view;
import math.vector;
import math.random;

export inline void Blaster_Fire(entity &ent, vector g_offset, int32_t damage, bool hyper, entity_effects effect)
{
	vector	forward, right;
	vector	start;
	vector	offset;

	if (is_quad)
		damage *= damage_multiplier;
	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);
	offset = { 24, 8, ent.viewheight - 8.f };
	offset += g_offset;
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	ent.client->kick_origin = forward * -2;
	ent.client->kick_angles[0] = -1.f;

	fire_blaster(ent, start, forward, damage, 1000, effect, hyper ? MOD_HYPERBLASTER : MOD_BLASTER, hyper);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t) ent.s.number);
	if (hyper)
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte(MZ_BLASTER | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif
}

static void Weapon_Blaster_Fire(entity &ent)
{
#ifdef SINGLE_PLAYER
	int32_t damage;
	if (!deathmatch)
		damage = 10;
	else
		damage = 15;

#else
	constexpr int32_t damage = 15;
#endif
	Blaster_Fire(ent, vec3_origin, damage, false, EF_BLASTER);
	ent.client->ps.gunframe++;
}

export void Weapon_Blaster(entity &ent)
{
	Weapon_Generic(ent, 4, 8, 52, 55, { 19, 32 }, { 5 }, Weapon_Blaster_Fire);
}