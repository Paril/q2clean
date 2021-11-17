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
	const int32_t damage = (!deathmatch ? 150 : 100) * damage_multiplier;
	const int32_t kick = (!deathmatch ? 250 : 200) * damage_multiplier;
#else
	constexpr int32_t damage = 100 * damage_multiplier;
	constexpr int32_t kick = 200 * damage_multiplier;
#endif

	vector forward, right;
	AngleVectors(ent.client.v_angle, &forward, &right, nullptr);

	ent.client.kick_origin = forward * -3;
	ent.client.kick_angles[0] = -3.f;

	vector offset = { 0, 7, ent.viewheight - 8.f };
	vector start = P_ProjectSource(ent, ent.origin, offset, forward, right);
	fire_rail(ent, start, forward, damage, kick);

	// send muzzle flash
	gi.ConstructMessage(svc_muzzleflash, ent, MZ_RAILGUN | is_silenced).multicast(ent.origin, MULTICAST_PVS);

	ent.client.ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client.pers.inventory[ent.client.ammo_index]--;
}

void Weapon_Railgun(entity &ent)
{
	Weapon_Generic(ent, 3, 18, 56, 61, G_FrameIsOneOf<56>, G_FrameIsOneOf<4>, weapon_railgun_fire);
}
