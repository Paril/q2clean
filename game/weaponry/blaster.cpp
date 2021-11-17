#include "config.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/entity.h"
#include "game/combat.h"
#include "game/ballistics/blaster.h"
#include "blaster.h"

void Blaster_Fire(entity &ent, vector g_offset, int32_t damage, bool hyper, entity_effects effect)
{
	vector	forward, right;
	vector	start;
	vector	offset;

	AngleVectors(ent.client.v_angle, &forward, &right, nullptr);
	offset = { 24, 8, ent.viewheight - 8.f };
	offset += g_offset;
	start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	ent.client.kick_origin = forward * -2;
	ent.client.kick_angles[0] = -1.f;

	fire_blaster(ent, start, forward, damage, 1000, effect, hyper);

	// send muzzle flash
	gi.ConstructMessage(svc_muzzleflash, ent, (hyper ? MZ_HYPERBLASTER : MZ_BLASTER) | is_silenced).multicast(ent.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif
}

static void Weapon_Blaster_Fire(entity &ent)
{
#ifdef SINGLE_PLAYER
	const int32_t damage = (!deathmatch ? 10 : 15) * damage_multiplier;
#else
	constexpr int32_t damage = 15 * damage_multiplier;
#endif
	Blaster_Fire(ent, vec3_origin, damage, false, EF_BLASTER);
	ent.client.ps.gunframe++;
}

void Weapon_Blaster(entity &ent)
{
	Weapon_Generic(ent, 4, 8, 52, 55, G_FrameIsOneOf<19, 32>, G_FrameIsOneOf<5>, Weapon_Blaster_Fire);
}