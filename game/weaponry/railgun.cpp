#include "config.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/rail.h"
#include "railgun.h"

static void weapon_railgun_fire(entity &ent)
{
#ifdef SINGLE_PLAYER
	int32_t	damage;
	int32_t	kick;

	if (deathmatch)
	{
		// normal damage is too extreme in dm
		damage = 100;
		kick = 200;
	}
	else
	{
		damage = 150;
		kick = 250;
	}
#else
	int32_t damage = 100;
	int32_t kick = 200;
#endif

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vector forward, right;
	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);

	ent.client->kick_origin = forward * -3;
	ent.client->kick_angles[0] = -3.f;

	vector offset = { 0, 7, ent.viewheight - 8.f };
	vector start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);
	fire_rail(ent, start, forward, damage, kick);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t) ent.s.number);
	gi.WriteByte(MZ_RAILGUN | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;
}

void Weapon_Railgun(entity &ent)
{
	Weapon_Generic(ent, 3, 18, 56, 61, G_IsAnyFrame<56>, G_IsAnyFrame<4>, weapon_railgun_fire);
}
