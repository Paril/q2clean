#include "config.h"
#include "game/entity_types.h"
#include "lib/protocol.h"
#include "game/combat.h"
#include "lib/types.h"
#include "game/weaponry.h"
#include "game/ballistics.h"
#include "lib/gi.h"
#include "game/util.h"

constexpr spawn_flag BLASTER_IS_HYPER = (spawn_flag) 1;

/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
void blaster_touch(entity &self, entity &other, vector normal, const surface &surf)
{
	if (other == self.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(self);
		return;
	}
#ifdef SINGLE_PLAYER

	if (self.owner->is_client())
		PlayerNoise(self.owner, self.s.origin, PNOISE_IMPACT);
#endif

	if (other.takedamage)
		T_Damage(other, self, self.owner, self.velocity, self.s.origin, normal, self.dmg, 1, DAMAGE_ENERGY, (means_of_death) self.sounds);
	else
	{
		gi.WriteByte(svc_temp_entity);
#ifdef THE_RECKONING
		if (self.s.effects & EF_BLUEHYPERBLASTER)	// Knightmare- this was checking bit TE_BLUEHYPERBLASTER
			gi.WriteByte(TE_FLECHETTE);			// Knightmare- TE_BLUEHYPERBLASTER is broken (parse error) in most Q2 engines
		else
#endif
			gi.WriteByte(TE_BLASTER);
		gi.WritePosition(self.s.origin);
		gi.WriteDir(normal);
		gi.multicast(self.s.origin, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

REGISTER_SAVABLE_FUNCTION(blaster_touch);

void fire_blaster(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity_effects effect, means_of_death mod, bool hyper)
{
	VectorNormalize(dir);

	entity &bolt = G_Spawn();
	bolt.svflags = SVF_DEADMONSTER;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	bolt.s.origin = start;
	bolt.s.old_origin = start;
	bolt.s.angles = vectoangles(dir);
	bolt.velocity = dir * speed;
	bolt.movetype = MOVETYPE_FLYMISSILE;
	bolt.clipmask = MASK_SHOT;
	bolt.solid = SOLID_BBOX;
	bolt.s.effects |= effect;
	bolt.mins = vec3_origin;
	bolt.maxs = vec3_origin;
#ifdef THE_RECKONING
	if (effect & EF_BLUEHYPERBLASTER)
		bolt.s.modelindex = gi.modelindex("models/objects/blaser/tris.md2");
	else
#endif
		bolt.s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	bolt.s.sound = gi.soundindex("misc/lasfly.wav");
	bolt.owner = self;
	bolt.touch = blaster_touch_savable;
	bolt.nextthink = level.framenum + 2 * BASE_FRAMERATE;
	bolt.think = G_FreeEdict_savable;
	bolt.dmg = damage;
	bolt.type = ET_BLASTER_BOLT;
	if (hyper)
		bolt.spawnflags = BLASTER_IS_HYPER;
	bolt.sounds = mod;
	gi.linkentity(bolt);

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge(self, bolt.s.origin, dir, speed);
#endif

	trace tr = gi.traceline(self.s.origin, bolt.s.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0f)
	{
		bolt.s.origin += (-10 * dir);
		bolt.touch(bolt, tr.ent, vec3_origin, null_surface);
	}
}