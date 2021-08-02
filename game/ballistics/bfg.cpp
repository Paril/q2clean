#include "config.h"
#include "game/entity_types.h"
#include "lib/protocol.h"
#include "game/combat.h"
#include "lib/types.h"
#include "game/weaponry.h"
#include "game/ballistics.h"
#include "lib/gi.h"
#include "game/util.h"
#include "game/cmds.h"
#include "game/misc.h"
#include "bfg.h"

static void bfg_explode(entity &self)
{
	if (self.s.frame == 0)
	{
		// the BFG effect
		for (entity &ent : G_IterateRadius(self.s.origin, self.dmg_radius))
		{
			if (!ent.takedamage)
				continue;
			if (ent == self.owner)
				continue;
			if (!CanDamage(ent, self))
				continue;
			if (!CanDamage(ent, self.owner))
				continue;

			vector v = ent.s.origin + ent.bounds.center();
			v = self.s.origin - v;
			float dist = VectorLength(v);
			float points = self.radius_dmg * (1.0f - sqrt(dist / self.dmg_radius));
			if (ent == self.owner)
				points *= 0.5f;

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BFG_EXPLOSION);
			gi.WritePosition(ent.s.origin);
			gi.multicast(ent.s.origin, MULTICAST_PHS);
			T_Damage(ent, self, self.owner, self.velocity, ent.s.origin, vec3_origin, (int32_t) points, 0, DAMAGE_ENERGY, MOD_BFG_EFFECT);
		}
	}

	self.nextthink = level.framenum + 1;
	self.s.frame++;
	if (self.s.frame == 5)
		self.think = SAVABLE(G_FreeEdict);
}

static REGISTER_SAVABLE_FUNCTION(bfg_explode);

static void bfg_touch(entity &self, entity &other, vector normal, const surface &surf)
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

	// core explosion - prevents firing it into the wall/floor
	if (other.takedamage)
		T_Damage(other, self, self.owner, self.velocity, self.s.origin, normal, 200, 0, DAMAGE_NONE, MOD_BFG_BLAST);
	T_RadiusDamage(self, self.owner, 200, other, 100, MOD_BFG_BLAST);

	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bfg_.x1b.wav"), 1, ATTN_NORM, 0);
	self.solid = SOLID_NOT;
	self.touch = nullptr;
	self.s.origin += ((-1 * FRAMETIME) * self.velocity);
	self.velocity = vec3_origin;
	self.s.modelindex = gi.modelindex("sprites/s_bfg3.sp2");
	self.s.frame = 0;
	self.s.sound = SOUND_NONE;
	self.s.effects &= ~EF_ANIM_ALLFAST;
	self.think = SAVABLE(bfg_explode);
	self.nextthink = level.framenum + 1;
	self.enemy = other;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BFG_BIGEXPLOSION);
	gi.WritePosition(self.s.origin);
	gi.multicast(self.s.origin, MULTICAST_PVS);
}

static REGISTER_SAVABLE_FUNCTION(bfg_touch);

static void bfg_think(entity &self)
{
#ifdef SINGLE_PLAYER
	int32_t dmg;

	if (!deathmatch)
		dmg = 10;
	else
		dmg = 5;
#else
	const int32_t dmg = 5;
#endif

	for (entity &ent : G_IterateRadius(self.s.origin, 256))
	{
		if (ent == self)
			continue;

		if (ent == self.owner)
			continue;

		if (!ent.takedamage)
			continue;

		if (!(ent.svflags & SVF_MONSTER) && !ent.is_client()
#ifdef SINGLE_PLAYER
			&& ent.type != ET_MISC_EXPLOBOX
#endif
#ifdef GROUND_ZERO
			&& !(ent.flags & FL_DAMAGEABLE)
#endif
			)
			continue;

		if (OnSameTeam(self, ent))
			continue;

		vector point = ent.absbounds.center();
		vector dir = point - self.s.origin;
		VectorNormalize(dir);

		entityref ignore = self;
		vector start = self.s.origin;
		vector end = start + (2048 * dir);

		trace tr;

		while (1)
		{
			tr = gi.traceline(start, end, ignore, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

			if (tr.ent.is_world())
				break;

			// hurt it if we can
			if (tr.ent.takedamage && !(tr.ent.flags & FL_IMMUNE_LASER) && (tr.ent != self.owner))
				T_Damage(tr.ent, self, self.owner, dir, tr.endpos, vec3_origin, dmg, 1, DAMAGE_ENERGY, MOD_BFG_LASER);

			// if we hit something that's not a monster or player we're done
			if (!(tr.ent.svflags & SVF_MONSTER) && !tr.ent.is_client()
#ifdef GROUND_ZERO
				&& !(tr.ent.flags & FL_DAMAGEABLE)
#endif
				)
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(4);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.normal);
				gi.WriteByte((uint8_t) self.s.skinnum);
				gi.multicast(tr.endpos, MULTICAST_PVS);
				break;
			}

			ignore = tr.ent;
			start = tr.endpos;
		}

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BFG_LASER);
		gi.WritePosition(self.s.origin);
		gi.WritePosition(tr.endpos);
		gi.multicast(self.s.origin, MULTICAST_PHS);
	}

	self.nextthink = level.framenum + 1;
}

static REGISTER_SAVABLE_FUNCTION(bfg_think);

void fire_bfg(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius)
{
	entity &bfg = G_Spawn();
	bfg.s.origin = start;
	bfg.s.angles = vectoangles(dir);
	bfg.velocity = dir * speed;
	bfg.movetype = MOVETYPE_FLYMISSILE;
	bfg.clipmask = MASK_SHOT;
	bfg.solid = SOLID_BBOX;
	bfg.s.effects |= EF_BFG | EF_ANIM_ALLFAST;
	bfg.s.modelindex = gi.modelindex("sprites/s_bfg1.sp2");
	bfg.owner = self;
	bfg.touch = SAVABLE(bfg_touch);
	bfg.nextthink = level.framenum + BASE_FRAMERATE * 8000 / speed;
	bfg.think = SAVABLE(G_FreeEdict);
	bfg.radius_dmg = damage;
	bfg.dmg_radius = damage_radius;
	bfg.s.sound = gi.soundindex("weapons/bfg__l1a.wav");

	bfg.think = SAVABLE(bfg_think);
	bfg.nextthink = level.framenum + 1;

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge(self, bfg.s.origin, dir, speed);
#endif

	gi.linkentity(bfg);
}