#include "config.h"
#include "game/entity_types.h"
#include "lib/protocol.h"
#include "game/combat.h"
#include "lib/types.h"
#include "game/weaponry.h"
#include "game/ballistics.h"
#include "lib/gi.h"
#include "game/util.h"
#include "blaster.h"

constexpr spawn_flag BLASTER_IS_HYPER = (spawn_flag) 1;

constexpr means_of_death MOD_TARGET_BLASTER { .self_kill_fmt = "{0} got blasted.\n" };

constexpr means_of_death MOD_BLASTER { .other_kill_fmt = "{0} was blasted by {3}.\n" };
constexpr means_of_death MOD_HYPERBLASTER { .other_kill_fmt = "{0} was melted by {3}'s hyperblaster.\n" };

inline means_of_death_ref blaster_mod(entity &self)
{
	if (self.owner.has_value() && (self.owner->is_client || (self.owner->svflags & SVF_MONSTER)))
	{
		if (self.spawnflags & BLASTER_IS_HYPER)
			return MOD_HYPERBLASTER;

		return MOD_BLASTER;
	}

	return MOD_TARGET_BLASTER;
}

/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
static void blaster_touch(entity &self, entity &other, vector normal, const surface &surf)
{
	if (other == self.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(self);
		return;
	}
#ifdef SINGLE_PLAYER

	if (self.owner->is_client)
		PlayerNoise(self.owner, self.origin, PNOISE_IMPACT);
#endif

	means_of_death_ref mod = blaster_mod(self);

	if (other.takedamage)
#ifdef GROUND_ZERO
	{
		if (self.radius_dmg)
		{
			bool damagestat = false;

			if (self.owner.has_value())
			{
				damagestat = self.owner->takedamage;
				self.owner->takedamage = false;
			}

			if (self.dmg >= 5)
				T_RadiusDamage(self, self.owner, self.dmg*3, other, self.dmg_radius, mod);

			T_Damage (other, self, self.owner, self.velocity, self.origin, normal, self.dmg, 1, { DAMAGE_ENERGY }, mod);

			if (self.owner.has_value())
				self.owner->takedamage = damagestat;
		}
		else
#endif
		T_Damage(other, self, self.owner, self.velocity, self.origin, normal, self.dmg, 1, { DAMAGE_ENERGY }, mod);
#ifdef GROUND_ZERO
	}
#endif
	else
	{
#ifdef GROUND_ZERO
		if (self.radius_dmg && self.dmg >= 5)
			T_RadiusDamage(self, self.owner, self.dmg*3, self.owner, self.dmg_radius, mod);

#endif
		temp_event hit_event =
#ifdef THE_RECKONING
			(self.effects & EF_BLUEHYPERBLASTER) ? TE_FLECHETTE :
#endif
			TE_BLASTER;

		gi.ConstructMessage(svc_temp_entity, hit_event, self.origin, vecdir { normal }).multicast(self.origin, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

REGISTER_STATIC_SAVABLE(blaster_touch);

void fire_blaster(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity_effects effect, bool hyper)
{
	VectorNormalize(dir);

	entity &bolt = G_Spawn();
	bolt.svflags = SVF_DEADMONSTER;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	bolt.origin = start;
	bolt.old_origin = start;
	bolt.angles = vectoangles(dir);
	bolt.velocity = dir * speed;
	bolt.movetype = MOVETYPE_FLYMISSILE;
	bolt.clipmask = MASK_SHOT;
	bolt.solid = SOLID_BBOX;
#ifdef THE_RECKONING
	if (effect & EF_BLUEHYPERBLASTER)
		bolt.modelindex = gi.modelindex("models/objects/blaser/tris.md2");
	else
#endif
#ifdef GROUND_ZERO
	if (effect & EF_TRACKER)
	{
		bolt.modelindex = gi.modelindex("models/proj/laser2/tris.md2");
		bolt.dmg_radius = 128.f;

		if (!(effect & ~EF_TRACKER))
			effect = EF_NONE;
	}
	else
#endif
		bolt.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	bolt.effects |= effect;
	bolt.sound = gi.soundindex("misc/lasfly.wav");
	bolt.owner = self;
	bolt.touch = SAVABLE(blaster_touch);
	bolt.nextthink = level.time + 2s;
	bolt.think = SAVABLE(G_FreeEdict);
	bolt.dmg = damage;
	if (hyper)
		bolt.spawnflags = BLASTER_IS_HYPER;
	gi.linkentity(bolt);

#ifdef SINGLE_PLAYER
	if (self.is_client)
		check_dodge(self, bolt.origin, dir, speed);
#endif

	trace tr = gi.traceline(self.origin, bolt.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0f)
	{
		bolt.origin += -10 * dir;
		bolt.touch(bolt, tr.ent, vec3_origin, null_surface);
	}
}