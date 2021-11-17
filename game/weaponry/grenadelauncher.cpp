#include "config.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/grenade.h"
#include "game/entity.h"
#include "game/game.h"
#include "grenadelauncher.h"

#ifdef GROUND_ZERO
#include "game/rogue/ballistics/prox.h"
#endif

static void weapon_grenadelauncher_fire(entity &ent)
{
	vector	offset;
	vector	forward, right;
	vector	start;
#ifdef GROUND_ZERO
	bool	is_prox = ent.client.pers.weapon->id == ITEM_PROX_LAUNCHER;
	const int32_t damage = (is_prox ? 90 : 120) * damage_multiplier;
#else
	const int32_t damage = 120 * damage_multiplier;
#endif
	const float radius = (damage / damage_multiplier) + 40.f;

	offset = { 8, 8, ent.viewheight - 8.f };
	AngleVectors(ent.client.v_angle, &forward, &right, nullptr);
	start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	ent.client.kick_origin = forward * -2;
	ent.client.kick_angles[0] = -1.f;

#ifdef GROUND_ZERO
	if (is_prox)
		fire_prox(ent, start, forward, damage_multiplier, 600);
	else
#endif
		fire_grenade(ent, start, forward, damage, 600, 2.5s, radius);

	gi.ConstructMessage(svc_muzzleflash, ent, MZ_GRENADE | is_silenced).multicast(ent.origin, MULTICAST_PVS);

	ent.client.ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client.pers.inventory[ent.client.ammo_index]--;
}

void Weapon_GrenadeLauncher(entity &ent)
{
	Weapon_Generic(ent, 5, 16, 59, 64, G_FrameIsOneOf<34, 51, 59>, G_FrameIsOneOf<6>, weapon_grenadelauncher_fire);
}
