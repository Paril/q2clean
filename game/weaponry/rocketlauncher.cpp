#include "config.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/rocket.h"
#include "rocketlauncher.h"

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
	start = P_ProjectSource(ent, ent.origin, offset, forward, right);
	fire_rocket(ent, start, forward, damage, 650, damage_radius, radius_damage);

	// send muzzle flash
	gi.ConstructMessage(svc_muzzleflash, ent, MZ_ROCKET | is_silenced).multicast(ent.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;
}

void Weapon_RocketLauncher(entity &ent)
{
	Weapon_Generic(ent, 4, 12, 50, 54, G_FrameIsOneOf<25, 33, 42, 50>, G_FrameIsOneOf<5>, Weapon_RocketLauncher_Fire);
}