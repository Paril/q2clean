#include "config.h"

#if defined(THE_RECKONING) && defined(SINGLE_PLAYER)
#include "game/entity.h"
#include "game/game.h"

#include "lib/gi.h"
#include "game/util.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "game/ballistics/blaster.h"
#include "game/ballistics/rocket.h"
#include "game/xatrix/ballistics/plasma.h"
#include "game/xatrix/ballistics/ionripper.h"
#include "game/target.h"
#include "monster.h"

void monster_fire_blueblaster(entity &self, vector start, vector dir, int32_t damage, int32_t speed, monster_muzzleflash, entity_effects effect)
{
	fire_blaster(self, start, dir, damage, speed, effect, false);

	gi.ConstructMessage(svc_muzzleflash, self, MZ_HYPERBLASTER).multicast(start, MULTICAST_PVS);
}

void monster_fire_ionripper(entity &self, vector start, vector dir, int32_t damage, int32_t speed, monster_muzzleflash flashtype, entity_effects effect)
{
	fire_ionripper(self, start, dir, damage, speed, effect);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

static void heat_think(entity &self)
{
	float	oldlen = 0;
	entityref	acquire;

	// acquire new target
	for (entity &target : G_IterateRadius(self.origin, 1024))
	{
		if (self.owner == target)
			continue;
		if ((target.svflags & SVF_MONSTER) || !target.is_client)
			continue;
		if (target.health <= 0)
			continue;
		if (!visible (self, target))
			continue;
		if (!infront (self, target))
			continue;

		vector vec = self.origin - target.origin;
		float len = VectorLength (vec);

		if (!acquire.has_value() || len < oldlen)
		{
			acquire = target;
			oldlen = len;
		}
	}

	if (acquire.has_value())
	{
		vector vec = acquire->origin - self.origin;
		VectorNormalize(vec);
		
		self.angles = vectoangles(vec);
		self.movedir = vec;
		self.velocity = vec * 500;
	}

	self.nextthink = level.time + 1_hz;
}

REGISTER_STATIC_SAVABLE(heat_think);

inline void fire_heat(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius, int32_t radius_damage)
{
	entity &heat = fire_rocket(self, start, dir, damage, speed, damage_radius, radius_damage);
	heat.nextthink = level.time + 1_hz;
	heat.think = SAVABLE(heat_think);
}

void monster_fire_heat(entity &self, vector start, vector dir, int32_t damage, int32_t speed, monster_muzzleflash flashtype)
{
	fire_heat(self, start, dir, damage, speed, damage, damage);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

static void dabeam_hit(entity &self)
{
	entityref ignore = self;
	vector start = self.origin;
	vector end = start + (self.movedir * 2048);
	trace tr;

	do
	{
		tr = gi.traceline(start, end, ignore, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

		if (tr.ent.is_world())
			break;

		// hurt it if we can
		if ((tr.ent.takedamage) && !(tr.ent.flags & FL_IMMUNE_LASER) && (tr.ent != self.owner))
			T_Damage(tr.ent, self, self.owner, self.movedir, tr.endpos, vec3_origin, self.dmg, (int32_t) skill, { DAMAGE_ENERGY }, MOD_TARGET_LASER);

		if (self.dmg < 0) // healer ray
			// when player is at 100 health
			// just undo health fix
			// keeping fx
			if (tr.ent.is_client && tr.ent.health > 100)
				tr.ent.health += self.dmg;

		// if we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(tr.ent.svflags & SVF_MONSTER) && (!tr.ent.is_client))
		{
			if (self.spawnflags & (spawn_flag) 0x80000000)
			{
				self.spawnflags &= (spawn_flag) ~0x80000000;
				gi.ConstructMessage(svc_temp_entity, TE_LASER_SPARKS, (uint8_t) 10, tr.endpos, vecdir { tr.normal }, (uint8_t) self.skinnum).multicast(tr.endpos, MULTICAST_PVS);
			}

			break;
		}

		ignore = tr.ent;
		start = tr.endpos;
	} while (1);

	self.old_origin = tr.endpos;
	self.nextthink = level.time + 1_hz;
	self.think = SAVABLE(G_FreeEdict);
}

REGISTER_STATIC_SAVABLE(dabeam_hit);

void monster_dabeam(entity &self)
{
	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_NOT;
	self.renderfx |= RF_BEAM | RF_TRANSLUCENT;
	self.modelindex = MODEL_WORLD;

	self.frame = 2;

	if (self.owner->monsterinfo.aiflags & AI_MEDIC)
		self.skinnum = 0xf3f3f1f1;
	else
		self.skinnum = 0xf2f2f0f0;

	if (self.enemy.has_value())
	{
		vector last_movedir = self.movedir;
		vector point = self.enemy->absbounds.center();

		if (self.owner->monsterinfo.aiflags & AI_MEDIC)
			point[0] += sin(gtimef(level.time).count()) * 8;

		self.movedir = point - self.origin;
		VectorNormalize(self.movedir);

		if (self.movedir != last_movedir)
			self.spawnflags |= (spawn_flag) 0x80000000;
	}
	else
		G_SetMovedir(self.angles, self.movedir);

	self.think = SAVABLE(dabeam_hit);
	self.nextthink = level.time + 1_hz;
	self.bounds = bbox::sized(8.f);
	gi.linkentity(self);

	self.spawnflags |= (spawn_flag) 0x80000001;
	self.svflags &= ~SVF_NOCLIENT;
}

#endif