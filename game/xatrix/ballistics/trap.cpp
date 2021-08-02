#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/game.h"
#include "game/util.h"
#include "lib/math.h"
#include "lib/math/random.h"
#include "game/combat.h"
#include "game/ballistics/grenade.h"
#ifdef SINGLE_PLAYER
#include "game/move.h"
#endif
#include "game/items/entity.h"
#include "game/xatrix/items.h"
#include "game/xatrix/monster/gekk.h"
#include "trap.h"

static void Trap_Gib_Think(entity &ent)
{
	if (ent.owner->s.frame != 5)
	{
		G_FreeEdict(ent);
		return;
	}

	vector forward;
	AngleVectors(ent.s.angles, &forward, nullptr, nullptr);

	float angle = deg2rad(ent.dmg_radius + ent.owner->delay) * 4.f;
	ent.s.origin = (vector { -cos(angle), sin(angle), 0 } * (ent.owner->wait * 0.5f)) + ent.owner->s.origin + forward;
	ent.s.origin[2] = ent.owner->s.origin[2] + ent.owner->wait;

	gi.linkentity(ent);

	ent.nextthink = level.framenum + 1;
}

static REGISTER_SAVABLE_FUNCTION(Trap_Gib_Think);

static void Trap_Think(entity &ent)
{
	if (ent.timestamp < level.framenum)
	{
		BecomeExplosion1(ent);
		return;
	}

	ent.nextthink = level.framenum + 1;

	if (ent.groundentity == null_entity)
		return;

	// ok lets do the blood effect
	if (ent.s.frame > 4)
	{
		if (ent.s.frame == 5)
		{
			if (ent.wait == 64)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/trapdown.wav"), 1, ATTN_IDLE, 0);

				for (int32_t i = 0; i < 3; i++)
				{
					entity &best = G_Spawn();

#ifdef SINGLE_PLAYER
					if (ent.enemy->type == ET_MONSTER_GEKK)
					{
						best.s.modelindex = gi.modelindex("models/objects/gekkgib/torso/tris.md2");
						best.s.effects |= EF_GREENGIB;
					}
					else
#endif
					if (ent.enemy->mass > 200)
					{
						best.s.modelindex = gi.modelindex("models/objects/gibs/chest/tris.md2");
						best.s.effects |= EF_GIB;
					}
					else
					{
						best.s.modelindex = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
						best.s.effects |= EF_GIB;
					}

					best.dmg_radius = (360.0f / 3) * i;
					best.think = SAVABLE(Trap_Gib_Think);
					best.nextthink = level.framenum + 1;
					best.s.angles = ent.s.angles;
					best.solid = SOLID_NOT;
					best.takedamage = true;
					best.movetype = MOVETYPE_NONE;
					best.svflags |= SVF_MONSTER;
					best.deadflag = DEAD_DEAD;
					best.owner = ent;
					best.watertype = gi.pointcontents(best.s.origin);
					if (best.watertype & MASK_WATER)
						best.waterlevel = 1;

					Trap_Gib_Think(best);
				}
			}

			ent.wait -= 2;
			ent.delay += level.time;

			if (ent.wait < 19)
				ent.s.frame++;

			return;
		}
		ent.s.frame++;
		if (ent.s.frame == 8)
		{
			ent.nextthink = level.framenum + 1 * BASE_FRAMERATE;
			ent.think = SAVABLE(G_FreeEdict);

			entity &best = G_Spawn();
			SpawnItem(best, GetItemByIndex(ITEM_FOODCUBE));
			best.spawnflags |= DROPPED_ITEM;
			best.s.origin = ent.s.origin;
			best.s.origin[2] += 16;
			best.velocity.z = 400.f;
			best.count = ent.mass;
			gi.linkentity(best);
			return;
		}
		return;
	}

	ent.s.effects &= ~EF_TRAP;
	if (ent.s.frame >= 4)
	{
		ent.s.effects |= EF_TRAP;
		ent.bounds = bbox_point;
	}

	if (ent.s.frame < 4)
		ent.s.frame++;

	entityref	target, best;
	float		oldlen = 8000;

	while ((target = findradius(target, ent.s.origin, 256)).has_value())
	{
		if (target == ent)
			continue;
		if (!(target->svflags & SVF_MONSTER) && !target->is_client())
			continue;
		if (target->health <= 0)
			continue;
		if (!visible(ent, target))
			continue;
		if (!best.has_value())
		{
			best = target;
			continue;
		}
		vector vec = ent.s.origin - target->s.origin;
		float len = VectorLength(vec);
		if (len < oldlen)
		{
			oldlen = len;
			best = target;
		}
	}

	// pull the enemy in
	if (!best.has_value())
		return;

	if (best->groundentity.has_value())
	{
		best->s.origin[2] += 1;
		best->groundentity = null_entity;
	}

	vector vec = ent.s.origin - best->s.origin;
	float len = VectorLength(vec);

#ifdef SINGLE_PLAYER
	if (best->is_client())
	{
#endif
		VectorNormalize(vec);
		best->velocity += vec * 250;
#ifdef SINGLE_PLAYER
	}
	else
	{
		vector forward;
		best->ideal_yaw = vectoyaw(vec);
		M_ChangeYaw(best);
		AngleVectors(best->s.angles, &forward, nullptr, nullptr);
		best->velocity = forward * 256;
	}
#endif

	gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/trapsuck.wav"), 1, ATTN_IDLE, 0);

	if (len >= 32)
		return;

	if (best->mass < 400)
	{
		T_Damage(best, ent, ent.owner, vec3_origin, best->s.origin, vec3_origin, 100000, 1, DAMAGE_NO_PROTECTION, MOD_TRAP);
		ent.enemy = best;
		ent.wait = 64.f;
		ent.s.old_origin = ent.s.origin;
		ent.timestamp = level.framenum + 30 * BASE_FRAMERATE;
#ifdef SINGLE_PLAYER

		if (!deathmatch)
			ent.mass = (int) (best->mass * 0.1f);
		else
#endif
			ent.mass = (int) (best->mass * 0.25f);

		ent.s.frame = 5;
		return;
	}

	BecomeExplosion1(ent);
}

static REGISTER_SAVABLE_FUNCTION(Trap_Think);

// RAFAEL
void fire_trap(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, float timer, float damage_radius, bool held)
{
	vector dir = vectoangles(aimdir);
	vector right, up;
	AngleVectors(dir, nullptr, &right, &up);

	entity &trap = G_Spawn();
	trap.s.origin = start;
	trap.velocity = aimdir * speed;
	trap.velocity += random(190.f, 210.f) * up;
	trap.velocity += random(-10.f, 10.f) * right;
	trap.avelocity = { 0, 300, 0 };
	trap.movetype = MOVETYPE_BOUNCE;
	trap.clipmask = MASK_SHOT;
	trap.solid = SOLID_BBOX;
	trap.bounds = {
		.mins = { 4, -4, 0 },
		.maxs = { 4, 4, 8 }
	};
	trap.s.modelindex = gi.modelindex("models/weapons/z_trap/tris.md2");
	trap.owner = self;
	trap.nextthink = level.framenum + 1 * BASE_FRAMERATE;
	trap.think = SAVABLE(Trap_Think);
	trap.dmg = damage;
	trap.dmg_radius = damage_radius;
	trap.type = ET_GRENADE;
	trap.s.sound = gi.soundindex("weapons/traploop.wav");
	if (held)
		trap.spawnflags = (spawn_flag) 3;
	else
		trap.spawnflags = (spawn_flag) 1;

	if (timer <= 0.0)
		Grenade_Explode(trap);
	else
		gi.linkentity(trap);

	trap.timestamp = level.framenum + 30 * BASE_FRAMERATE;
}
#endif