#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "game/ballistics.h"
#include "game/combat.h"
#include "lib/math/random.h"
#include "lib/gi.h"
#include "plasma.h"

constexpr means_of_death MOD_PHALANX { .other_kill_fmt = "{0} was evaporated by {3}.\n" };

static void plasma_touch(entity &ent, entity &other, vector normal, const surface &surf)
{
	if (other == ent.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(ent);
		return;
	}
#if defined(SINGLE_PLAYER)

	if (ent.owner->is_client)
		PlayerNoise(ent.owner, ent.origin, PNOISE_IMPACT);
#endif

	// calculate position for the explosion entity
	vector origin = ent.origin + (ent.velocity * -0.02f);

	if (other.takedamage)
		T_Damage(other, ent, ent.owner, ent.velocity, ent.origin, normal, ent.dmg, 0, { DAMAGE_NONE }, MOD_PHALANX);

	T_RadiusDamage(ent, ent.owner, ent.radius_dmg, other, ent.dmg_radius, MOD_PHALANX);

	gi.ConstructMessage(svc_temp_entity, TE_PLASMA_EXPLOSION, origin).multicast(ent.origin, MULTICAST_PVS);

	G_FreeEdict(ent);
}

REGISTER_STATIC_SAVABLE(plasma_touch);

void fire_plasma(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius, int32_t radius_damage)
{
	entity &plasma = G_Spawn();
	plasma.origin = start;
	plasma.movedir = dir;
	plasma.angles = vectoangles(dir);
	plasma.velocity = dir * speed;
	plasma.movetype = MOVETYPE_FLYMISSILE;
	plasma.clipmask = MASK_SHOT;
	plasma.solid = SOLID_BBOX;

	plasma.owner = self;
	plasma.touch = SAVABLE(plasma_touch);
	plasma.nextthink = level.time + seconds(8000 / speed);
	plasma.think = SAVABLE(G_FreeEdict);
	plasma.dmg = damage;
	plasma.radius_dmg = radius_damage;
	plasma.dmg_radius = damage_radius;
	plasma.sound = gi.soundindex("weapons/rockfly.wav");

	plasma.modelindex = gi.modelindex("sprites/s_photon.sp2");
	plasma.effects |= EF_PLASMA | EF_ANIM_ALLFAST;

#ifdef SINGLE_PLAYER
	if (self.is_client)
		check_dodge(self, plasma.origin, dir, speed);
#endif

	gi.linkentity(plasma);
}
#endif