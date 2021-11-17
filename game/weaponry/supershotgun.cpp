#include "config.h"
#include "game/weaponry.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/bullet.h"
#include "supershotgun.h"

constexpr means_of_death MOD_SSHOTGUN { .other_kill_fmt = "{0} was blown away by {3}'s super shotgun.\n" };

static void weapon_supershotgun_fire(entity &ent)
{
	vector	start;
	vector	forward, right;
	vector	offset;
	vector	v;
	const int32_t damage = 6 * damage_multiplier;
	const int32_t kick = 12 * damage_multiplier;

	AngleVectors(ent.client.v_angle, &forward, &right, nullptr);

	ent.client.kick_origin = forward * -2;
	ent.client.kick_angles[0] = -2.f;

	offset = { 0, 8,  ent.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	v[PITCH] = ent.client.v_angle[PITCH];
	v[YAW] = ent.client.v_angle[YAW] - 5;
	v[ROLL] = ent.client.v_angle[ROLL];
	AngleVectors(v, &forward, nullptr, nullptr);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
	v[YAW] = ent.client.v_angle[YAW] + 5;
	AngleVectors(v, &forward, nullptr, nullptr);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);

	// send muzzle flash
	gi.ConstructMessage(svc_muzzleflash, ent, MZ_SSHOTGUN | is_silenced).multicast(ent.origin, MULTICAST_PVS);

	ent.client.ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client.pers.inventory[ent.client.ammo_index] -= 2;
}

void Weapon_SuperShotgun(entity &ent)
{
	Weapon_Generic(ent, 6, 17, 57, 61, G_FrameIsOneOf<29, 42, 57>, G_FrameIsOneOf<7>, weapon_supershotgun_fire);
}
