#include "config.h"
#include "game/weaponry.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/bullet.h"

static void weapon_shotgun_fire(entity &ent)
{
	vector	start;
	vector	forward, right;
	vector	offset;
	int32_t	damage = 4;
	int32_t	kick = 8;

	if (ent.client->ps.gunframe == 9)
	{
		ent.client->ps.gunframe++;
		return;
	}

	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);

	ent.client->kick_origin = forward * -2;
	ent.client->kick_angles[0] = -2.f;

	offset = { 0, 8,  ent.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t) ent.s.number);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags) dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;
}

void Weapon_Shotgun(entity &ent)
{
	Weapon_Generic(ent, 7, 18, 36, 39, { 22, 28, 34 }, { 8, 9 }, weapon_shotgun_fire);
}