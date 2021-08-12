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

constexpr means_of_death MOD_BFG_BLAST { .self_kill_fmt = "{0} should have used a smaller gun.\n", .other_kill_fmt = "{0} was disintegrated by {3}'s BFG blast.\n" };
constexpr means_of_death MOD_BFG_LASER { .other_kill_fmt = "{0} saw the pretty lights from {3}'s BFG.\n" };
constexpr means_of_death MOD_BFG_EFFECT { .other_kill_fmt = "{0} couldn't hide from {3}'s BFG.\n" };

static void bfg_explode(entity &self)
{
	if (self.frame == 0)
	{
		// the BFG effect
		for (entity &ent : G_IterateRadius(self.origin, self.dmg_radius))
		{
			if (!ent.takedamage)
				continue;
			if (ent == self.owner)
				continue;
			if (!CanDamage(ent, self))
				continue;
			if (!CanDamage(ent, self.owner))
				continue;

			vector v = ent.origin + ent.bounds.center();
			v = self.origin - v;
			float dist = VectorLength(v);
			float points = self.radius_dmg * (1.0f - sqrt(dist / self.dmg_radius));
			if (ent == self.owner)
				points *= 0.5f;

			gi.ConstructMessage(svc_temp_entity, TE_BFG_EXPLOSION, ent.origin).multicast(ent.origin, MULTICAST_PHS);
			T_Damage(ent, self, self.owner, self.velocity, ent.origin, vec3_origin, (int32_t) points, 0, { DAMAGE_ENERGY}, MOD_BFG_EFFECT);
		}
	}

	self.nextthink = level.framenum + 1;
	self.frame++;
	if (self.frame == 5)
		self.think = SAVABLE(G_FreeEdict);
}

REGISTER_STATIC_SAVABLE(bfg_explode);

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
		PlayerNoise(self.owner, self.origin, PNOISE_IMPACT);
#endif

	// core explosion - prevents firing it into the wall/floor
	if (other.takedamage)
		T_Damage(other, self, self.owner, self.velocity, self.origin, normal, 200, 0, { DAMAGE_NONE }, MOD_BFG_BLAST);
	T_RadiusDamage(self, self.owner, 200, other, 100, MOD_BFG_BLAST);

	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bfg_.x1b.wav"));
	self.solid = SOLID_NOT;
	self.touch = nullptr;
	self.origin += ((-1 * FRAMETIME) * self.velocity);
	self.velocity = vec3_origin;
	self.modelindex = gi.modelindex("sprites/s_bfg3.sp2");
	self.frame = 0;
	self.sound = SOUND_NONE;
	self.effects &= ~EF_ANIM_ALLFAST;
	self.think = SAVABLE(bfg_explode);
	self.nextthink = level.framenum + 1;
	self.enemy = other;

	gi.ConstructMessage(svc_temp_entity, TE_BFG_BIGEXPLOSION, self.origin).multicast(self.origin, MULTICAST_PVS);
}

REGISTER_STATIC_SAVABLE(bfg_touch);

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

	for (entity &ent : G_IterateRadius(self.origin, 256))
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
		vector dir = point - self.origin;
		VectorNormalize(dir);

		entityref ignore = self;
		vector start = self.origin;
		vector end = start + (2048 * dir);

		trace tr;

		while (1)
		{
			tr = gi.traceline(start, end, ignore, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

			if (tr.ent.is_world())
				break;

			// hurt it if we can
			if (tr.ent.takedamage && !(tr.ent.flags & FL_IMMUNE_LASER) && (tr.ent != self.owner))
				T_Damage(tr.ent, self, self.owner, dir, tr.endpos, vec3_origin, dmg, 1, { DAMAGE_ENERGY}, MOD_BFG_LASER);

			// if we hit something that's not a monster or player we're done
			if (!(tr.ent.svflags & SVF_MONSTER) && !tr.ent.is_client()
#ifdef GROUND_ZERO
				&& !(tr.ent.flags & FL_DAMAGEABLE)
#endif
				)
			{
				gi.ConstructMessage(svc_temp_entity, TE_LASER_SPARKS, (uint8_t) 4, tr.endpos, vecdir { tr.normal }, (uint8_t) self.skinnum).multicast(tr.endpos, MULTICAST_PVS);
				break;
			}

			ignore = tr.ent;
			start = tr.endpos;
		}

		gi.ConstructMessage(svc_temp_entity, TE_BFG_LASER, self.origin, tr.endpos).multicast(self.origin, MULTICAST_PHS);
	}

	self.nextthink = level.framenum + 1;
}

REGISTER_STATIC_SAVABLE(bfg_think);

void fire_bfg(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius)
{
	entity &bfg = G_Spawn();
	bfg.origin = start;
	bfg.angles = vectoangles(dir);
	bfg.velocity = dir * speed;
	bfg.movetype = MOVETYPE_FLYMISSILE;
	bfg.clipmask = MASK_SHOT;
	bfg.solid = SOLID_BBOX;
	bfg.effects |= EF_BFG | EF_ANIM_ALLFAST;
	bfg.modelindex = gi.modelindex("sprites/s_bfg1.sp2");
	bfg.owner = self;
	bfg.touch = SAVABLE(bfg_touch);
	bfg.nextthink = level.framenum + BASE_FRAMERATE * 8000 / speed;
	bfg.think = SAVABLE(G_FreeEdict);
	bfg.radius_dmg = damage;
	bfg.dmg_radius = damage_radius;
	bfg.sound = gi.soundindex("weapons/bfg__l1a.wav");

	bfg.think = SAVABLE(bfg_think);
	bfg.nextthink = level.framenum + 1;

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge(self, bfg.origin, dir, speed);
#endif

	gi.linkentity(bfg);
}