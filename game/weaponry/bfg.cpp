#include "config.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/bfg.h"
#include "game/entity.h"
#include "game/game.h"

static void weapon_bfg_fire(entity &ent)
{
	vector	offset, start;
	vector	forward, right;
	float	damage_radius = 1000.f;
#ifdef SINGLE_PLAYER
	int32_t	damage;

	if (!deathmatch)
		damage = 500;
	else
		damage = 200;
#else
	int32_t	damage = 200;
#endif

	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);

	offset = { 8, 8, ent.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	if (ent.client->ps.gunframe == 9)
	{
		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort((int16_t) ent.s.number);
		gi.WriteByte(MZ_BFG | is_silenced);
		gi.multicast(ent.s.origin, MULTICAST_PVS);

		ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

		PlayerNoise(ent, start, PNOISE_WEAPON);
#endif
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (ent.client->pers.inventory[ent.client->ammo_index] < 50)
	{
		ent.client->ps.gunframe++;
		return;
	}

	ent.client->kick_origin = forward * -2;

	// make a big pitch kick with an inverse fall
	ent.client->v_dmg_pitch = -40.f;
	ent.client->v_dmg_roll = random(-8.f, 8.f);
	ent.client->v_dmg_time = level.time + DAMAGE_TIME;

	if (is_quad)
		damage *= damage_multiplier;
	fire_bfg(ent, start, forward, damage, 400, damage_radius);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags) dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index] -= 50;
}

void Weapon_BFG(entity &ent)
{
	Weapon_Generic(ent, 8, 32, 55, 58, { 39, 45, 50, 55 }, { 9, 17 }, weapon_bfg_fire);
}
