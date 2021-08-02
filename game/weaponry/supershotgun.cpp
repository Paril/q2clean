#include "config.h"
#include "game/weaponry.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/bullet.h"
#include "supershotgun.h"

static void weapon_supershotgun_fire(entity &ent)
{
	vector	start;
	vector	forward, right;
	vector	offset;
	vector	v;
	int32_t	damage = 6;
	int32_t	kick = 12;

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

	v[PITCH] = ent.client->v_angle[PITCH];
	v[YAW] = ent.client->v_angle[YAW] - 5;
	v[ROLL] = ent.client->v_angle[ROLL];
	AngleVectors(v, &forward, nullptr, nullptr);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
	v[YAW] = ent.client->v_angle[YAW] + 5;
	AngleVectors(v, &forward, nullptr, nullptr);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t) ent.s.number);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index] -= 2;
}

void Weapon_SuperShotgun(entity &ent)
{
	Weapon_Generic(ent, 6, 17, 57, 61, G_IsAnyFrame<29, 42, 57>, G_IsAnyFrame<7>, weapon_supershotgun_fire);
}
