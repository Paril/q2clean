#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "lib/math/random.h"
#include "game/xatrix/ballistics/ionripper.h"
#include "ionripper.h"

static void weapon_ionripper_fire(entity &ent)
{
#ifdef SINGLE_PLAYER
	int32_t	damage;
	int32_t	kick;

	if (!deathmatch)
	{
		damage = 50;
		kick = 60;
	}
	else
	{
		// tone down for deathmatch
		damage = 30;
		kick = 40;
	}
#else
	int damage = 30;
	int kick = 40;
#endif

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vector tempang = ent.client->v_angle;
	tempang[YAW] += crandom();

	vector forward, right;
	AngleVectors(tempang, &forward, &right, nullptr);

	ent.client->kick_origin = forward * -3;
	ent.client->kick_angles[0] = -3;

	vector offset { 16, 7, ent.viewheight - 8.f };
	vector start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	fire_ionripper(ent, start, forward, damage, 500, EF_IONRIPPER);

	// send muzzle flash
	gi.ConstructMessage(svc_muzzleflash, ent, MZ_IONRIPPER | is_silenced).multicast(ent.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index] -= ent.client->pers.weapon->quantity;

	if (ent.client->pers.inventory[ent.client->ammo_index] < 0)
		ent.client->pers.inventory[ent.client->ammo_index] = 0;
}

void Weapon_Ionripper(entity &ent)
{
	Weapon_Generic(ent, 4, 6, 36, 39, G_FrameIsOneOf<36>, G_FrameIsOneOf<5>, weapon_ionripper_fire);
}
#endif