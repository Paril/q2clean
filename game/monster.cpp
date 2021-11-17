#include "config.h"

#ifdef SINGLE_PLAYER
#include "entity.h"
#include "game.h"
#include "move.h"
#include "ai.h"

#include "lib/gi.h"
#include "game/util.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "game/ballistics/bullet.h"
#include "game/ballistics/blaster.h"
#include "game/ballistics/grenade.h"
#include "game/ballistics/rocket.h"
#include "game/ballistics/rail.h"
#include "game/ballistics/bfg.h"
#include "game/items/entity.h"
#include "combat.h"
#include "misc.h"

#ifdef GROUND_ZERO
#include "game/xatrix/monster/fixbot.h"
#endif

//
// monster weapons
//

void monster_fire_bullet(entity &self, vector start, vector dir, int damage, int kick, int hspread, int vspread, monster_muzzleflash flashtype)
{
	fire_bullet(self, start, dir, damage, kick, hspread, vspread, MOD_DEFAULT);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

void monster_fire_shotgun(entity &self, vector start, vector aimdir, int damage, int kick, int hspread, int vspread, int count, monster_muzzleflash flashtype)
{
	fire_shotgun(self, start, aimdir, damage, kick, hspread, vspread, count, MOD_DEFAULT);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

void monster_fire_blaster(entity &self, vector start, vector dir, int damage, int speed, monster_muzzleflash flashtype, entity_effects effect)
{
	fire_blaster(self, start, dir, damage, speed, effect, false);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

void monster_fire_grenade(entity &self, vector start, vector aimdir, int damage, int speed, monster_muzzleflash flashtype)
{
	fire_grenade(self, start, aimdir, damage, speed, 2.5s, damage + 40);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

void monster_fire_rocket(entity &self, vector start, vector dir, int damage, int speed, monster_muzzleflash flashtype)
{
	fire_rocket(self, start, dir, damage, speed, damage + 20, damage);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

void monster_fire_railgun(entity &self, vector start, vector aimdir, int damage, int kick, monster_muzzleflash flashtype)
{
	fire_rail(self, start, aimdir, damage, kick);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

void monster_fire_bfg(entity &self, vector start, vector aimdir, int damage, int speed, float damage_radius, monster_muzzleflash flashtype)
{
	fire_bfg(self, start, aimdir, damage, speed, damage_radius);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

//
// Monster utility functions
//

void M_FliesOff(entity &self)
{
	self.effects &= ~EF_FLIES;
	self.sound = SOUND_NONE;
}

REGISTER_SAVABLE(M_FliesOff);

void M_FliesOn(entity &self)
{
	if (self.waterlevel)
		return;
	self.effects |= EF_FLIES;
	self.sound = gi.soundindex("infantry/inflies1.wav");
	self.think = SAVABLE(M_FliesOff);
	self.nextthink = level.time + 1min;
}

REGISTER_SAVABLE(M_FliesOn);

void M_FlyCheck(entity &self)
{
	if (self.waterlevel)
		return;

	if (random() > 0.5f)
		return;

	self.think = SAVABLE(M_FliesOn);
	self.nextthink = level.time + random(5s, 15s);
}

void M_CheckGround(entity &ent)
{
	vector	point;
	trace	tr;

	if (ent.flags & (FL_SWIM | FL_FLY))
		return;

#ifdef ROGUE_AI
	if ((ent.velocity.z * ent.gravityVector.z) < -100)		// PGM
#else
	if (ent.velocity.z > 100)
#endif
	{
		ent.groundentity = null_entity;
		return;
	}

// if the hull point one-quarter unit down is solid the entity is on ground
	point.x = ent.origin[0];
	point.y = ent.origin[1];
#ifdef ROGUE_AI
	point.z = ent.origin[2] + (0.25f * ent.gravityVector[2]);	//PGM
#else
	point.z = ent.origin[2] - 0.25f;
#endif

	tr = gi.trace(ent.origin, ent.bounds, point, ent, MASK_MONSTERSOLID);

	// check steepness
#ifdef ROGUE_AI
	if ((ent.gravityVector[2] < 0 ? (tr.normal[2] < 0.7f) : (tr.normal[2] > -0.7f)) && !tr.startsolid)
#else
	if (tr.normal[2] < 0.7f && !tr.startsolid)
#endif
	{
		ent.groundentity = null_entity;
		return;
	}

	if (!tr.startsolid && !tr.allsolid) {
		ent.origin = tr.endpos;
		ent.groundentity = tr.ent;
		ent.groundentity_linkcount = tr.ent.linkcount;
		ent.velocity.z = 0;
	}
}

void M_CatagorizePosition(entity &ent)
{
//
// get waterlevel
//
	vector point = ent.origin;
	point.z += ent.bounds.mins.z + 1;
	content_flags cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
	{
		ent.waterlevel = WATER_NONE;
		ent.watertype = CONTENTS_NONE;
		return;
	}

	ent.watertype = cont;
	ent.waterlevel = WATER_LEGS;
	point.z += 26;
	cont = gi.pointcontents(point);
	if (!(cont & MASK_WATER))
		return;

	ent.waterlevel = WATER_WAIST;
	point.z += 22;
	cont = gi.pointcontents(point);
	if (cont & MASK_WATER)
		ent.waterlevel = WATER_UNDER;
}

void M_WorldEffects(entity &ent)
{
	int32_t dmg;

	if (ent.health > 0) {
		if (!(ent.flags & FL_SWIM)) {
			if (ent.waterlevel < WATER_UNDER) {
				ent.air_finished_time = level.time + 12s;
			} else if (ent.air_finished_time < level.time) {
				// drown!
				if (ent.pain_debounce_time < level.time) {
					dmg = (int32_t)(2 + 2 * ((level.time - ent.air_finished_time).count() / BASE_FRAMERATE));
					if (dmg > 15)
						dmg = 15;
					T_Damage(ent, world, world, vec3_origin, ent.origin, vec3_origin, dmg, 0, { DAMAGE_NO_ARMOR }, MOD_WATER);
					ent.pain_debounce_time = level.time + 1s;
				}
			}
		} else {
			if (ent.waterlevel > WATER_NONE) {
				ent.air_finished_time = level.time + 9s;
			} else if (ent.air_finished_time < level.time) {
				// suffocate!
				if (ent.pain_debounce_time < level.time) {
					dmg = (int32_t)(2 + 2 * ((level.time - ent.air_finished_time).count() / BASE_FRAMERATE));
					if (dmg > 15)
						dmg = 15;
					T_Damage(ent, world, world, vec3_origin, ent.origin, vec3_origin, dmg, 0, { DAMAGE_NO_ARMOR }, MOD_WATER);
					ent.pain_debounce_time = level.time + 1s;
				}
			}
		}
	}

	if (ent.waterlevel == 0) {
		if (ent.flags & FL_INWATER) {
			gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"));
			ent.flags &= ~FL_INWATER;
		}
		return;
	}

	if ((ent.watertype & CONTENTS_LAVA) && !(ent.flags & FL_IMMUNE_LAVA)) {
		if (ent.damage_debounce_time < level.time) {
			ent.damage_debounce_time = level.time + 200ms;
			T_Damage(ent, world, world, vec3_origin, ent.origin, vec3_origin, 10 * ent.waterlevel, 0, { DAMAGE_NONE }, MOD_LAVA);
		}
	}
	if ((ent.watertype & CONTENTS_SLIME) && !(ent.flags & FL_IMMUNE_SLIME)) {
		if (ent.damage_debounce_time < level.time) {
			ent.damage_debounce_time = level.time + 1s;
			T_Damage(ent, world, world, vec3_origin, ent.origin, vec3_origin, 4 * ent.waterlevel, 0, { DAMAGE_NONE }, MOD_SLIME);
		}
	}

	if (!(ent.flags & FL_INWATER)) {
		if (!(ent.svflags & SVF_DEADMONSTER)) {
			if (ent.watertype & CONTENTS_LAVA)
				if (random() <= 0.5f)
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava1.wav"));
				else
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"));
			else if (ent.watertype & CONTENTS_SLIME)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"));
			else if (ent.watertype & CONTENTS_WATER)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"));
		}

		ent.flags |= FL_INWATER;
		ent.damage_debounce_time = gtime::zero();
	}
}

void M_droptofloor(entity &ent)
{
	vector	end;
	trace	tr;

#ifdef ROGUE_AI
	float grav = 1.0f - ent.gravityVector[2];
	ent.origin[2] += 1 * grav;
	end = ent.origin;
	end.z -= 256 * grav;
#else
	ent.origin[2] += 1;
	end = ent.origin;
	end.z -= 256;
#endif

	tr = gi.trace(ent.origin, ent.bounds, end, ent, MASK_MONSTERSOLID);

	if (tr.fraction == 1 || tr.allsolid)
		return;

	ent.origin = tr.endpos;

	gi.linkentity(ent);
	M_CheckGround(ent);
	M_CatagorizePosition(ent);
}

REGISTER_SAVABLE(M_droptofloor);

void M_SetEffects(entity &ent)
{
	ent.effects &= ~(EF_COLOR_SHELL | EF_POWERSCREEN);
	ent.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);

	if (ent.monsterinfo.aiflags & AI_RESURRECTING) {
		ent.effects |= EF_COLOR_SHELL;
		ent.renderfx |= RF_SHELL_RED;
	}

	if (ent.health <= 0)
		return;

	if (ent.powerarmor_time > level.time) {
		if (ent.monsterinfo.power_armor_type == ITEM_POWER_SCREEN) {
			ent.effects |= EF_POWERSCREEN;
		} else if (ent.monsterinfo.power_armor_type == ITEM_POWER_SHIELD) {
			ent.effects |= EF_COLOR_SHELL;
			ent.renderfx |= RF_SHELL_GREEN;
		}
	}

#ifdef GROUND_ZERO
	ent.effects &= ~(EF_DOUBLE | EF_QUAD | EF_PENT);
	ent.renderfx &= ~RF_SHELL_DOUBLE;

	if (ent.monsterinfo.quad_time > level.time)
	{
		gtime remaining = ent.monsterinfo.quad_time - level.time;
		if (remaining > 3s || (remaining % 8ms) >= 4ms)
			ent.effects |= EF_QUAD;
	}
	else
		ent.effects &= ~EF_QUAD;

	if (ent.monsterinfo.double_time > level.time)
	{
		gtime remaining = ent.monsterinfo.double_time - level.time;
		if (remaining > 3s || (remaining % 8ms) >= 4ms)
			ent.effects |= EF_DOUBLE;
	}
	else
		ent.effects &= ~EF_DOUBLE;

	if (ent.monsterinfo.invincible_time > level.time)
	{
		gtime remaining = ent.monsterinfo.invincible_time - level.time;
		if (remaining > 3s || (remaining % 8ms) >= 4ms)
			ent.effects |= EF_PENT;
	}
	else
		ent.effects &= ~EF_PENT;
#endif
}

#ifdef ROGUE_AI
void cleanupHealTarget(entity &ent)
{
	ent.monsterinfo.healer = 0;
	ent.takedamage = true;
	ent.monsterinfo.aiflags &= ~AI_RESURRECTING;
	M_SetEffects (ent);
}
#endif

struct m_animparameters
{
	bool reverse;
	uint16_t start, end;

	m_animparameters(const mmove_t *move) :
		reverse(move->lastframe < move->firstframe),
		start(reverse ? move->lastframe : move->firstframe),
		end(reverse ? move->firstframe : move->lastframe)
	{
	}
};

static void M_MoveFrame(entity &self)
{
	const mmove_t *move = self.monsterinfo.currentmove;
	self.nextthink = level.time + 100ms;

	m_animparameters params { move };

	if ((self.monsterinfo.nextframe) && (self.monsterinfo.nextframe >= params.start) && (self.monsterinfo.nextframe <= params.end)) {
		self.frame = self.monsterinfo.nextframe;
		self.monsterinfo.nextframe = 0;
	} else {
		if (self.frame == move->lastframe) {
			if (move->endfunc) {
				move->endfunc(self);

				// regrab move, endfunc is very likely to change it
				move = self.monsterinfo.currentmove;
				params = move;
				
				// check for death
				if (self.svflags & SVF_DEADMONSTER)
					return;
			}
		}

		if (self.frame < params.start || self.frame > params.end) {
			self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self.frame = move->firstframe;
		} else {
			if (!(self.monsterinfo.aiflags & AI_HOLD_FRAME)) {
				if (params.reverse)
				{
					self.frame--;

					if (self.frame < params.start)
						self.frame = params.end;
				}
				else
				{
					self.frame++;

					if (self.frame > params.end)
						self.frame = params.start;
				}
			}
		}
	}

	int index = abs(self.frame - move->firstframe);
	
	const mframe_t &frame = move->frames[index];
	
	if (frame.aifunc) {
		if (!(self.monsterinfo.aiflags & AI_HOLD_FRAME))
			frame.aifunc(self, frame.dist * self.monsterinfo.scale);
		else
			frame.aifunc(self, 0);
	}

	if (frame.thinkfunc)
		frame.thinkfunc(self);
}

#ifdef GROUND_ZERO
#include "game/rogue/ballistics/tesla.h"
#include "rogue/ai.h"
#endif

// temporary
static entity_type ET_MONSTER_TANK, ET_MONSTER_SUPERTANK, ET_MONSTER_MAKRON, ET_MONSTER_JORG;

inline void M_HandleDamage(entity &targ, entity &attacker, entity &inflictor [[maybe_unused]])
{
	if (!(attacker.is_client) && !(attacker.svflags & SVF_MONSTER))
		return;

#ifdef GROUND_ZERO
	// logic for tesla - if you are hit by a tesla, and can't see who you should be mad at (attacker)
	// attack the tesla
	// also, target the tesla if it's a "new" tesla
	if (inflictor.type == ET_TESLA)
	{
#ifdef ROGUE_AI
		bool new_tesla = MarkTeslaArea(inflictor);
		if (new_tesla)
#endif
			TargetTesla(targ, inflictor);
		return;
	}
#endif

	if (attacker == targ || attacker == targ.enemy)
		return;

	// if we are a good guy monster and our attacker is a player
	// or another good guy, do not get mad at them
	if (targ.monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (attacker.is_client || (attacker.monsterinfo.aiflags & AI_GOOD_GUY))
			return;
	}

#ifdef GROUND_ZERO
	// if we're currently mad at something a target_anger made us mad at, ignore
	// damage
	if (targ.enemy.has_value() && (targ.monsterinfo.aiflags & AI_TARGET_ANGER))
	{
		float	percentHealth;

		// make sure whatever we were pissed at is still around.
		if (targ.enemy->inuse)
		{
			percentHealth = (float) (targ.health) / (float) (targ.max_health);

			if (targ.enemy->inuse && percentHealth > 0.33)
				return;
		}

		// remove the target anger flag
		targ.monsterinfo.aiflags &= ~AI_TARGET_ANGER;
	}
#endif

#ifdef ROGUE_AI
	// if we're healing someone, do like above and try to stay with them
	if (targ.enemy.has_value() && (targ.monsterinfo.aiflags & AI_MEDIC))
	{
		float	percentHealth = (float) (targ.health) / (float) (targ.max_health);

		// ignore it some of the time
		if (targ.enemy->inuse && percentHealth > 0.25)
			return;

		// remove the medic flag
		targ.monsterinfo.aiflags &= ~AI_MEDIC;
		cleanupHealTarget(targ.enemy);
	}
#endif

	// we now know that we are not both good guys

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker.is_client)
	{
		targ.monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ.enemy.has_value() && targ.enemy->is_client)
		{
			if (visible(targ, targ.enemy))
			{
				targ.oldenemy = attacker;
				return;
			}
			targ.oldenemy = targ.enemy;
		}
		targ.enemy = attacker;
		if (!(targ.monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
		return;
	}

	// it's the same base (walk/swim/fly) type and a different classname and it's not a tank
	// (they spray too much), get mad at them
	if (((targ.flags & (FL_FLY | FL_SWIM)) == (attacker.flags & (FL_FLY | FL_SWIM))) &&
		!targ.type.is_exact(attacker.type) &&
#if defined(ROGUE_AI) || defined(GROUND_ZERO)
		!(attacker.monsterinfo.aiflags & AI_IGNORE_SHOTS) &&
		!(targ.monsterinfo.aiflags & AI_IGNORE_SHOTS))
#else
		(attacker.type != ET_MONSTER_TANK) &&
		(attacker.type != ET_MONSTER_SUPERTANK) &&
		(attacker.type != ET_MONSTER_MAKRON) &&
		(attacker.type != ET_MONSTER_JORG))
#endif
	{
		if (targ.enemy.has_value() && targ.enemy->is_client)
			targ.oldenemy = targ.enemy;
		targ.enemy = attacker;
		if (!(targ.monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
	// if they *meant* to shoot us, then shoot back
	else if (attacker.enemy == targ)
	{
		if (targ.enemy.has_value() && targ.enemy->is_client)
			targ.oldenemy = targ.enemy;
		targ.enemy = attacker;
		if (!(targ.monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker.enemy.has_value() && attacker.enemy != targ)
	{
		if (targ.enemy.has_value() && targ.enemy->is_client)
			targ.oldenemy = targ.enemy;
		targ.enemy = attacker.enemy;
		if (!(targ.monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
}

void M_ReactToDamage(entity &targ, entity &attacker, entity &inflictor, int32_t knockback, int32_t take)
{
	// Switch targets if applicable
	M_HandleDamage(targ, attacker, inflictor);

	// Call custom reaction function
	if (targ.monsterinfo.reacttodamage)
		targ.monsterinfo.reacttodamage(targ, attacker, inflictor, knockback, take);
}

void monster_think(entity &self)
{
	M_MoveFrame(self);
	if (self.linkcount != self.monsterinfo.linkcount)
	{
		self.monsterinfo.linkcount = self.linkcount;
		M_CheckGround(self);
	}
	M_CatagorizePosition(self);
	M_WorldEffects(self);
	M_SetEffects(self);
}

REGISTER_SAVABLE(monster_think);

void monster_use(entity &self, entity &, entity &cactivator)
{
	if (self.enemy.has_value())
		return;
	if (self.health <= 0)
		return;
	if (cactivator.flags & FL_NOTARGET)
		return;
	if (!(cactivator.is_client) && !(cactivator.monsterinfo.aiflags & AI_GOOD_GUY))
		return;
#ifdef GROUND_ZERO
	if (cactivator.flags & FL_DISGUISED)
		return;
#endif

// delay reaction so if the monster is teleported, its sound is still heard
	self.enemy = cactivator;
	FoundTarget(self);
}

REGISTER_SAVABLE(monster_use);

// from monster.qc
static void monster_start_go(entity &self);

static void monster_triggered_spawn(entity &self)
{
	self.origin[2] += 1;
	KillBox(self);

	self.solid = SOLID_BBOX;
	self.movetype = MOVETYPE_STEP;
	self.svflags &= ~SVF_NOCLIENT;
	self.air_finished_time = level.time + 12s;
	gi.linkentity(self);

	monster_start_go(self);

#ifdef THE_RECKONING
	// RAFAEL
	if (self.type == ET_MONSTER_FIXBOT)
	{
		if ((self.spawnflags & 16) || (self.spawnflags & 8) || (self.spawnflags & 4))
		{
			self.enemy = null_entity;
			return;
		}
	}
#endif

	if (self.enemy.has_value() && !(self.spawnflags & 1) && !(self.enemy->flags & FL_VISIBLE_MASK))
	{
#ifdef GROUND_ZERO
		if (!(self.enemy->flags & FL_DISGUISED))
#endif
			FoundTarget(self);
#ifdef GROUND_ZERO
		else
			self.enemy = null_entity;
#endif
	}
	else
		self.enemy = null_entity;
}

REGISTER_STATIC_SAVABLE(monster_triggered_spawn);

static void monster_triggered_spawn_use(entity &self, entity &, entity &cactivator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self.think = SAVABLE(monster_triggered_spawn);
	self.nextthink = level.time + 1_hz;
	if (cactivator.is_client)
		self.enemy = cactivator;
	self.use = SAVABLE(monster_use);
}

REGISTER_STATIC_SAVABLE(monster_triggered_spawn_use);

static void monster_triggered_start(entity &self)
{
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	self.svflags |= SVF_NOCLIENT;
	self.nextthink = gtime::zero();
	self.use = SAVABLE(monster_triggered_spawn_use);
}

void monster_death_use(entity &self)
{
	self.flags &= ~(FL_FLY | FL_SWIM);
	self.monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self.item) {
		Drop_Item(self, self.item);
		self.item = 0;
	}

	if (self.deathtarget)
		self.target = self.deathtarget;

	if (!self.target)
		return;

	G_UseTargets(self, self.enemy);
}

//============================================================================

void monster_pain(entity &self, entity &, float, int32_t)
{
	if (self.health < (self.max_health / 2))
		self.skinnum |= 1;
	else
		self.skinnum &= ~1;
}

REGISTER_SAVABLE(monster_pain);

static bool monster_start(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return false;
	}

	if ((self.spawnflags & (spawn_flag)4) && !(self.monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self.spawnflags &= ~(spawn_flag)4;
		self.spawnflags |= (spawn_flag)1;
	}

	if (!(self.monsterinfo.aiflags & AI_GOOD_GUY_MASK))
		level.total_monsters++;

	self.nextthink = level.time + 1_hz;
	self.svflags |= SVF_MONSTER;
	self.renderfx |= RF_FRAMELERP;
	self.takedamage = true;
	self.air_finished_time = level.time + 12s;
	self.use = SAVABLE(monster_use);
	self.max_health = self.health;
	self.clipmask = MASK_MONSTERSOLID;

	self.deadflag = false;
	self.svflags &= ~SVF_DEADMONSTER;

	if (!self.monsterinfo.checkattack)
		self.monsterinfo.checkattack = SAVABLE(M_CheckAttack);
	self.old_origin = self.origin;

	if (st.item)
	{
		self.item = FindItemByClassname(st.item);
		if (!self.item)
			gi.dprintfmt("{}: bad item \"{}\"\n", self, st.item);
	}

	// randomize what frame they start on
	const mmove_t *move = self.monsterinfo.currentmove;

	if (move)
		self.frame = move->firstframe + (Q_rand() % (move->lastframe - move->firstframe + 1));

#ifdef ROGUE_AI
	// PMM - get this so I don't have to do it in all of the monsters
	self.monsterinfo.base_height = self.bounds.maxs[2];
#endif

#ifdef GROUND_ZERO
	// PMM - clear these
	self.monsterinfo.quad_time = gtime::zero();
	self.monsterinfo.double_time = gtime::zero();
	self.monsterinfo.invincible_time = gtime::zero();
#endif

	// call pain to set up pain skin
	if (self.pain)
		self.pain(self, self, 0.f, 0);

	return true;
}

static void monster_start_go(entity &self)
{
	if (self.health <= 0)
		return;

	// check for target to combat_point and change to combattarget
	if (self.target)
	{
		bool notcombat = false, fixup = false;

		for (entity &ctarget : G_IterateFunc<&entity::targetname>(self.target, striequals))
		{
			if (ctarget.type == ET_POINT_COMBAT)
			{
				self.combattarget = self.target;
				fixup = true;
			}
			else
				notcombat = true;
		}

		if (notcombat && self.combattarget)
			gi.dprintfmt("{}: target with mixed types\n", self);
		if (fixup)
			self.target = nullptr;
	}

	// validate combattarget
	if (self.combattarget)
		for (entity &ctarget : G_IterateFunc<&entity::targetname>(self.combattarget, striequals))
			if (ctarget.type != ET_POINT_COMBAT)
				gi.dprintfmt("{}: bad combattarget \"{}\" ({})\n", self, self.combattarget, ctarget);

	if (self.target)
	{
		self.goalentity = self.movetarget = G_PickTarget(self.target);

		if (!self.movetarget.has_value())
		{
			gi.dprintfmt("{}: can't find target \"{}\"\n", self, self.target);
			self.target = nullptr;
			self.monsterinfo.pause_time = gtime::max();
			self.monsterinfo.stand(self);
		}
		else if (self.movetarget->type == ET_PATH_CORNER)
		{
			vector v = self.goalentity->origin - self.origin;
			self.ideal_yaw = self.angles[YAW] = vectoyaw(v);
			self.monsterinfo.walk(self);
			self.target = nullptr;
		}
		else
		{
			self.goalentity = self.movetarget = null_entity;
			self.monsterinfo.pause_time = gtime::max();
			self.monsterinfo.stand(self);
		}
	}
	else
	{
		self.monsterinfo.pause_time = gtime::max();
		self.monsterinfo.stand(self);
	}

	self.think = SAVABLE(monster_think);
	self.nextthink = level.time + 1_hz;
}

// temp
static entity_type ET_MONSTER_STALKER;

static void walkmonster_start_go(entity &self)
{
	if (!(self.spawnflags & 2) && level.time < 1s) {
		M_droptofloor(self);

		if (self.groundentity != null_entity)
			if (!M_walkmove(self, 0, 0))
				gi.dprintfmt("{}: in solid\n", self);
	}

	if (!self.yaw_speed)
		self.yaw_speed = 20.f;

#ifdef GROUND_ZERO
	// PMM - stalkers are too short for this
	if (self.type == ET_MONSTER_STALKER)
		self.viewheight = 15;
	else
#endif
		self.viewheight = 25;

	monster_start_go(self);

	if (self.spawnflags & (spawn_flag)2)
		monster_triggered_start(self);
}

REGISTER_STATIC_SAVABLE(walkmonster_start_go);

void walkmonster_start(entity &self)
{
	self.think = SAVABLE(walkmonster_start_go);
	monster_start(self);
}

static void flymonster_start_go(entity &self) 
{
	if (!M_walkmove(self, 0, 0))
		gi.dprintfmt("{}: in solid\n", self);

	if (!self.yaw_speed)
		self.yaw_speed = 10.f;
	self.viewheight = 25;

	monster_start_go(self);

	if (self.spawnflags & (spawn_flag)2)
		monster_triggered_start(self);
}

REGISTER_STATIC_SAVABLE(flymonster_start_go);

void flymonster_start(entity &self)
{
	self.flags |= FL_FLY;
	self.think = SAVABLE(flymonster_start_go);
	monster_start(self);
}

static void swimmonster_start_go(entity &self)
{
	if (!self.yaw_speed)
		self.yaw_speed = 10.f;
	self.viewheight = 10;

	monster_start_go(self);

	if (self.spawnflags & (spawn_flag)2)
		monster_triggered_start(self);
}

REGISTER_STATIC_SAVABLE(swimmonster_start_go);

void swimmonster_start(entity &self)
{
	self.flags |= FL_SWIM;
	self.think = SAVABLE(swimmonster_start_go);
	monster_start(self);
}

#ifdef GROUND_ZERO
static void stationarymonster_start_go(entity &self);

static void stationarymonster_triggered_spawn(entity &self)
{
	KillBox (self);

	self.solid = SOLID_BBOX;
	self.movetype = MOVETYPE_NONE;
	self.svflags &= ~SVF_NOCLIENT;
	self.air_finished_time = level.time + 12s;
	gi.linkentity (self);

	// FIXME - why doesn't this happen with real monsters?
	self.spawnflags &= (spawn_flag) ~2;

	stationarymonster_start_go(self);

	if (self.enemy.has_value() && !(self.spawnflags & (spawn_flag)1) && !(self.enemy->flags & FL_NOTARGET | FL_DISGUISED))
		FoundTarget (self);
	else
		self.enemy = 0;
}

REGISTER_STATIC_SAVABLE(stationarymonster_triggered_spawn);

static void stationarymonster_triggered_spawn_use(entity &self, entity &, entity &cactivator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self.think = SAVABLE(stationarymonster_triggered_spawn);
	self.nextthink = level.time + 1_hz;
	if (cactivator.is_client)
		self.enemy = cactivator;
	self.use = SAVABLE(monster_use);
}

REGISTER_STATIC_SAVABLE(stationarymonster_triggered_spawn_use);

static void stationarymonster_triggered_start(entity &self)
{
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	self.svflags |= SVF_NOCLIENT;
	self.nextthink = gtime::zero();
	self.use = SAVABLE(stationarymonster_triggered_spawn_use);
}

static void stationarymonster_start_go(entity &self)
{
	if (!self.yaw_speed)
		self.yaw_speed = 20.f;

	monster_start_go (self);

	if (self.spawnflags & 2)
		stationarymonster_triggered_start (self);
}

REGISTER_STATIC_SAVABLE(stationarymonster_start_go);

void stationarymonster_start(entity &self)
{
	self.think = SAVABLE(stationarymonster_start_go);
	monster_start (self);
}
#endif
#endif