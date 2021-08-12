#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "lib/math/random.h"
#include "game/xatrix/ballistics/plasma.h"
#include "phalanx.h"

static void weapon_phalanx_fire(entity &ent)
{
	int damage = 70 + (int32_t) (random(10.0));
	int radius_damage = 120;
	float damage_radius = 120.f;

	if (is_quad)
	{
		damage *= damage_multiplier;
		radius_damage *= damage_multiplier;
	}

	vector forward, right;
	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);

	ent.client->kick_origin = forward * -2;
	ent.client->kick_angles[0] = -2;

	vector offset { 0, 8, ent.viewheight - 8.f };
	vector start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	if (ent.client->ps.gunframe == 8)
	{
		vector v {	ent.client->v_angle[PITCH],
					ent.client->v_angle[YAW] - 1.5f,
					ent.client->v_angle[ROLL] };

		vector up;
		AngleVectors(v, &forward, &right, &up);

		radius_damage = 30;
		damage_radius = 120.f;

		fire_plasma(ent, start, forward, damage, 725, damage_radius, radius_damage);

		if (!(dmflags & DF_INFINITE_AMMO))
			ent.client->pers.inventory[ent.client->ammo_index]--;
	}
	else
	{
		vector v { ent.client->v_angle[PITCH],
					ent.client->v_angle[YAW] + 1.5f,
					ent.client->v_angle[ROLL] };

		vector up;
		AngleVectors(v, &forward, &right, &up);

		fire_plasma(ent, start, forward, damage, 725, damage_radius, radius_damage);

		// send muzzle flash
		gi.ConstructMessage(svc_muzzleflash, ent, MZ_PHALANX | is_silenced).multicast(ent.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

		PlayerNoise(ent, start, PNOISE_WEAPON);
#endif
	}

	ent.client->ps.gunframe++;
}

void Weapon_Phalanx(entity &ent)
{
	Weapon_Generic(ent, 5, 20, 58, 63, G_FrameIsOneOf<29, 42, 55>, G_FrameIsOneOf<7, 8>, weapon_phalanx_fire);
}
#endif