#include "config.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/bfg.h"
#include "game/entity.h"
#include "game/game.h"
#include "bfg.h"

static void weapon_bfg_fire(entity &ent)
{
	vector	offset, start;
	vector	forward, right;
	float	damage_radius = 1000.f;
#ifdef SINGLE_PLAYER
	const int32_t damage = (!deathmatch ? 500 : 200) * damage_multiplier;
#else
	constexpr int32_t damage = 200 * damage_multiplier;
#endif

	AngleVectors(ent.client.v_angle, &forward, &right, nullptr);

	offset = { 8, 8, ent.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	if (ent.client.ps.gunframe == 9)
	{
		// send muzzle flash
		gi.ConstructMessage(svc_muzzleflash, ent, MZ_BFG | is_silenced).multicast(ent.origin, MULTICAST_PVS);

		ent.client.ps.gunframe++;
#ifdef SINGLE_PLAYER

		PlayerNoise(ent, start, PNOISE_WEAPON);
#endif
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (ent.client.pers.inventory[ent.client.ammo_index] < 50)
	{
		ent.client.ps.gunframe++;
		return;
	}

	ent.client.kick_origin = forward * -2;

	// make a big pitch kick with an inverse fall
	ent.client.v_dmg_pitch = -40.f;
	ent.client.v_dmg_roll = crandom(8.f);
	ent.client.v_dmg_time = level.time + DAMAGE_TIME;

	fire_bfg(ent, start, forward, damage, 400, damage_radius);

	ent.client.ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client.pers.inventory[ent.client.ammo_index] -= 50;
}

void Weapon_BFG(entity &ent)
{
	Weapon_Generic(ent, 8, 32, 55, 58, G_FrameIsOneOf<39, 45, 50, 55>, G_FrameIsOneOf<9, 17>, weapon_bfg_fire);
}
