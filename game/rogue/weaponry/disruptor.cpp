#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "lib/math/random.h"
#include "disruptor.h"
#include "game/rogue/ballistics/tracker.h"

static void weapon_tracker_fire(entity &self)
{
#ifdef SINGLE_PLAYER
	const int32_t damage = (!deathmatch ? 45 : 30) * damage_multiplier;
#else
	constexpr int32_t damage = 30 * damage_multiplier;
#endif

	vector forward, right;
	AngleVectors (self.client.v_angle, &forward, &right, nullptr);

	vector offset { 24, 8, self.viewheight - 8.f };
	vector start = P_ProjectSource (self, self.origin, offset, forward, right);
	vector end = start + (forward * 8192);
	entityref enemy;

	trace tr = gi.traceline (start, end, self, MASK_SHOT);

	if (tr.ent.is_world())
		tr = gi.trace (start, bbox::sized(16.f), end, self, MASK_SHOT);

	if (!tr.ent.is_world())
		if ((tr.ent.svflags & SVF_MONSTER) || tr.ent.is_client || (tr.ent.flags & FL_DAMAGEABLE))
			if (tr.ent.health > 0)
				enemy = tr.ent;

	self.client.kick_origin = forward * -2;
	self.client.kick_angles[0] = -1.f;

	fire_tracker(self, start, forward, damage, 1000, enemy);

	// send muzzle flash
	gi.ConstructMessage(svc_muzzleflash, self, MZ_TRACKER).multicast(self.origin, MULTICAST_PVS);
#if defined(SINGLE_PLAYER)

	PlayerNoise(self, start, PNOISE_WEAPON);
#endif

	self.client.ps.gunframe++;
	if (!(dmflags & DF_INFINITE_AMMO))
		self.client.pers.inventory[self.client.ammo_index] -= self.client.pers.weapon->quantity;
}

void Weapon_Disruptor(entity &ent)
{
	Weapon_Generic(ent, 4, 9, 29, 34, G_FrameIsOneOf<14, 19, 23>, G_FrameIsOneOf<5>, weapon_tracker_fire);
}
#endif