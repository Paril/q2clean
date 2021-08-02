#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "game/ballistics.h"
#include "game/combat.h"
#include "lib/math/random.h"
#include "lib/gi.h"
#include "ionripper.h"

static void ionripper_sparks(entity &self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(0);
	gi.WritePosition(self.s.origin);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(0xe4 + (Q_rand() & 3));
	gi.multicast(self.s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

static REGISTER_SAVABLE_FUNCTION(ionripper_sparks);

static void ionripper_touch(entity &self, entity &other, vector normal, const surface &surf)
{
	if (other == self.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(self);
		return;
	}
#if defined(SINGLE_PLAYER)

	if (self.owner->is_client())
		PlayerNoise(self.owner, self.s.origin, PNOISE_IMPACT);
#endif

	if (other.takedamage)
	{
		T_Damage(other, self, self.owner, self.velocity, self.s.origin, normal, self.dmg, 1, DAMAGE_ENERGY, MOD_RIPPER);
		G_FreeEdict(self);
	}
}

static REGISTER_SAVABLE_FUNCTION(ionripper_touch);

void fire_ionripper(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity_effects effect)
{
	VectorNormalize(dir);

	entity &ion = G_Spawn();
	ion.s.origin = ion.s.old_origin = start;
	ion.s.angles = vectoangles(dir);
	ion.velocity = dir * speed;

	ion.movetype = MOVETYPE_WALLBOUNCE;
	ion.clipmask = MASK_SHOT;
	ion.solid = SOLID_BBOX;
	ion.s.effects |= effect;

	ion.s.renderfx |= RF_FULLBRIGHT;

	ion.s.modelindex = gi.modelindex("models/objects/boomrang/tris.md2");
	ion.s.sound = gi.soundindex("misc/lasfly.wav");
	ion.owner = self;
	ion.touch = SAVABLE(ionripper_touch);
	ion.nextthink = level.framenum + 3 * BASE_FRAMERATE;
	ion.think = SAVABLE(ionripper_sparks);
	ion.dmg = damage;
	ion.dmg_radius = 100.f;
	gi.linkentity(ion);

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge(self, ion.s.origin, dir, speed);
#endif

	trace tr = gi.traceline(self.s.origin, ion.s.origin, ion, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		ion.s.origin += dir * -10;
		ion.touch(ion, tr.ent, vec3_origin, null_surface);
	}
}
#endif