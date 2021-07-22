#include "entity.h"
#include "game.h"
#include "cmds.h"
#include "combat.h"
#include "util.h"
#include "lib/gi.h"
#include "items/armor.h"

bool CanDamage(entity &targ, entity &inflictor)
{
	// bmodels need special checking because their origin is 0,0,0
	if (targ.movetype == MOVETYPE_PUSH)
	{
		const vector dest = (targ.absmin + targ.absmax) * 0.5f;
		trace tr = gi.traceline(inflictor.s.origin, dest, inflictor, MASK_SOLID);

		if (tr.fraction == 1.0f)
			return true;
		if (tr.ent == targ)
			return true;

		return false;
	}

	trace tr = gi.traceline(inflictor.s.origin, targ.s.origin, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	vector dest = targ.s.origin;
	dest.x += 15.0f;
	dest.y += 15.0f;
	tr = gi.traceline(inflictor.s.origin, dest, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	dest = targ.s.origin;
	dest.x += 15.0f;
	dest.y -= 15.0f;
	tr = gi.traceline(inflictor.s.origin, dest, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	dest = targ.s.origin;
	dest.x -= 15.0f;
	dest.y += 15.0f;
	tr = gi.traceline(inflictor.s.origin, dest, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	dest = targ.s.origin;
	dest.x -= 15.0f;
	dest.y -= 15.0f;
	tr = gi.traceline(inflictor.s.origin, dest, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	return false;
}

means_of_death meansOfDeath;

bool CheckTeamDamage(means_of_death mod [[maybe_unused]])
{
	return ((dm_flags) dmflags & DF_NO_FRIENDLY_FIRE)
#ifdef GROUND_ZERO
		&& (mod != MOD_NUKE)
#endif
		;
}

void SpawnDamage(temp_event type, vector origin, vector normal)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WritePosition(origin);
	gi.WriteDir(normal);
	gi.multicast(origin, MULTICAST_PVS);
}

inline int32_t CheckPowerArmor(entity &ent, vector point, vector normal, int damage, damage_flags dflags)
{
	int32_t		save;
	gitem_id	power_armor_type;
	int32_t		damagePerCell;
	temp_event	pa_te_type;
	int32_t		power = 0;
	int32_t		power_used;

	if (!damage)
		return 0;

	if (dflags & (DAMAGE_NO_ARMOR
#ifdef GROUND_ZERO
		| DAMAGE_NO_POWER_ARMOR
#endif
		))
		return 0;

	if (ent.is_client())
	{
		power_armor_type = PowerArmorType(ent);
		if (power_armor_type != ITEM_NONE)
			power = ent.client->pers.inventory[ITEM_CELLS];
	}
#ifdef SINGLE_PLAYER
	else if (ent.svflags & SVF_MONSTER)
	{
		power_armor_type = ent.monsterinfo.power_armor_type;
		power = ent.monsterinfo.power_armor_power;
	}
#endif
	else
		return 0;

	if (!power || power_armor_type == ITEM_NONE)
		return 0;

	if (power_armor_type == ITEM_POWER_SCREEN)
	{
		vector	vec;
		float	dot;
		vector	forward;

		// only works if damage point is in front
		AngleVectors(ent.s.angles, &forward, nullptr, nullptr);
		vec = point - ent.s.origin;
		VectorNormalize(vec);
		dot = vec * forward;
		if (dot <= 0.3f)
			return 0;

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damagePerCell = 2;
		pa_te_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

#ifdef GROUND_ZERO
	if (dflags & DAMAGE_NO_REG_ARMOR)
		save = (power * damagePerCell) / 2;
	else
#endif
		save = power * damagePerCell;

	if (!save)
		return 0;
	if (save > damage)
		save = damage;

	SpawnDamage(pa_te_type, point, normal);
	ent.powerarmor_framenum = (gtime) (level.framenum + 0.2f * BASE_FRAMERATE);

#ifdef GROUND_ZERO
	if (dflags & DAMAGE_NO_REG_ARMOR)
		power_used = (save / damagePerCell) * 2;
	else
#endif
		power_used = save / damagePerCell;

#ifdef SINGLE_PLAYER
	if (ent.is_client())
#endif
		ent.client->pers.inventory[ITEM_CELLS] = max(0, ent.client->pers.inventory[ITEM_CELLS] - power_used);
#ifdef SINGLE_PLAYER
	else
		ent.monsterinfo.power_armor_power -= power_used;
#endif
	return save;
}

inline int32_t CheckArmor(entity &ent, vector point, vector normal, int32_t damage, temp_event te_sparks, damage_flags dflags)
{
	if (!damage)
		return 0;

	if (!ent.is_client())
		return 0;

	if (dflags & (DAMAGE_NO_ARMOR
#ifdef GROUND_ZERO
		| DAMAGE_NO_REG_ARMOR
#endif
		))
		return 0;

	gitem_id index = ArmorIndex(ent);

	if (!index)
		return 0;

	const gitem_armor &armor = GetItemByIndex(index).armor;

	int32_t save;

	if (dflags & DAMAGE_ENERGY)
		save = (int32_t) ceil(armor.energy_protection * damage);
	else
		save = (int32_t) ceil(armor.normal_protection * damage);

	if (save >= ent.client->pers.inventory[index])
		save = ent.client->pers.inventory[index];

	if (!save)
		return 0;

	ent.client->pers.inventory[index] -= save;
	SpawnDamage(te_sparks, point, normal);

	return save;
}

void Killed(entity &targ, entity &inflictor, entity &attacker, int32_t damage, vector point)
{
	if (targ.health < -999)
		targ.health = -999;

#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
	if (targ.monsterinfo.aiflags & AI_MEDIC)
	{
		if (targ.enemy)  // god, I hope so
			cleanupHealTarget(targ.enemy);

		// clean up self
		targ.monsterinfo.aiflags &= ~AI_MEDIC;
	}
#endif

	targ.enemy = attacker;

#ifdef SINGLE_PLAYER
	if ((targ.svflags & SVF_MONSTER) && (targ.deadflag != DEAD_DEAD))
	{
#ifdef GROUND_ZERO
		//ROGUE - free up slot for spawned monster if it's spawned
		if (targ.monsterinfo.aiflags & AI_SPAWNED_CARRIER)
		{
			if (targ.monsterinfo.commander && targ.monsterinfo.commander.inuse && targ.monsterinfo.commander.classname == "monster_carrier")
				targ.monsterinfo.commander.monsterinfo.monster_slots++;
		}
		if (targ.monsterinfo.aiflags & AI_SPAWNED_MEDIC_C)
		{
			if (targ.monsterinfo.commander && targ.monsterinfo.commander.inuse && targ.monsterinfo.commander.classname == "monster_medic_commander")
				targ.monsterinfo.commander.monsterinfo.monster_slots++;
		}
		if (targ.monsterinfo.aiflags & AI_SPAWNED_WIDOW)
		{
			// need to check this because we can have variable numbers of coop players
			if (targ.monsterinfo.commander && targ.monsterinfo.commander.inuse && !strncmp(targ.monsterinfo.commander.classname, "monster_widow", 13))
			{
				if (targ.monsterinfo.commander.monsterinfo.monster_used > 0)
					targ.monsterinfo.commander.monsterinfo.monster_used--;
			}
		}
#endif

		if (!(targ.monsterinfo.aiflags & AI_GOOD_GUY_MASK))
		{
			level.killed_monsters++;

			if ((int32_t) coop && attacker.is_client())
				attacker.client->resp.score++;

#ifndef GROUND_ZERO
			// medics won't heal monsters that they kill themselves
			if (attacker.type == ET_MONSTER_MEDIC)
				targ.owner = attacker;
#endif
		}
	}
#endif

	// doors, triggers, etc
	if (targ.movetype == MOVETYPE_PUSH || targ.movetype == MOVETYPE_STOP || targ.movetype == MOVETYPE_NONE)
	{
		targ.die(targ, inflictor, attacker, damage, point);
		return;
	}

#ifdef SINGLE_PLAYER
	if ((targ.svflags & SVF_MONSTER) && (targ.deadflag != DEAD_DEAD))
	{
		targ.touch = 0;
		monster_death_use(targ);
	}
#endif

	targ.die(targ, inflictor, attacker, damage, point);
}

#ifdef SINGLE_PLAYER
#include "ai.h"

#ifdef GROUND_ZERO
bool(entity self, entity tesla) MarkTeslaArea;
void(entity self, entity tesla) TargetTesla;

void(entity targ, entity attacker, entity inflictor) M_ReactToDamage =
#else
void M_ReactToDamage(entity &targ, entity &attacker)
#endif
{
	if (!(attacker.is_client()) && !(attacker.svflags & SVF_MONSTER))
		return;

#ifdef GROUND_ZERO
	// logic for tesla - if you are hit by a tesla, and can't see who you should be mad at (attacker)
	// attack the tesla
	// also, target the tesla if it's a "new" tesla
	if (inflictor && inflictor.classname == "tesla")
	{
		bool new_tesla = MarkTeslaArea(targ, inflictor);
		if (new_tesla)
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
		if (attacker.is_client() || (attacker.monsterinfo.aiflags & AI_GOOD_GUY))
			return;
	}

#ifdef GROUND_ZERO
	// if we're currently mad at something a target_anger made us mad at, ignore
	// damage
	if (targ.enemy && (targ.monsterinfo.aiflags & AI_TARGET_ANGER))
	{
		float	percentHealth;

		// make sure whatever we were pissed at is still around.
		if (targ.enemy.inuse)
		{
			percentHealth = (float) (targ.health) / (float) (targ.max_health);
			if (targ.enemy.inuse && percentHealth > 0.33)
				return;
		}

		// remove the target anger flag
		targ.monsterinfo.aiflags &= ~AI_TARGET_ANGER;
	}

	// if we're healing someone, do like above and try to stay with them
	if (targ.enemy && (targ.monsterinfo.aiflags & AI_MEDIC))
	{
		float	percentHealth;

		percentHealth = (float) (targ.health) / (float) (targ.max_health);
		// ignore it some of the time
		if (targ.enemy.inuse && percentHealth > 0.25)
			return;

		// remove the medic flag
		targ.monsterinfo.aiflags &= ~AI_MEDIC;
		cleanupHealTarget(targ.enemy);
	}
#endif

	// we now know that we are not both good guys

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker.is_client()) {
		targ.monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ.enemy.has_value() && targ.enemy->is_client()) {
			if (visible(targ, targ.enemy)) {
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
#ifdef GROUND_ZERO
	if (((targ.flags & (FL_FLY | FL_SWIM)) == (attacker.flags & (FL_FLY | FL_SWIM))) &&
		(targ.classname != attacker.classname) &&
		!(attacker.monsterinfo.aiflags & AI_IGNORE_SHOTS) &&
		!(targ.monsterinfo.aiflags & AI_IGNORE_SHOTS))
#else
	if (((targ.flags & (FL_FLY | FL_SWIM)) == (attacker.flags & (FL_FLY | FL_SWIM))) &&
		(targ.type != attacker.type) &&
		(attacker.type != ET_MONSTER_TANK) &&
		(attacker.type != ET_MONSTER_SUPERTANK) &&
		(attacker.type != ET_MONSTER_MAKRON) &&
		(attacker.type != ET_MONSTER_JORG))
#endif
	{
		if (targ.enemy.has_value() && targ.enemy->is_client())
			targ.oldenemy = targ.enemy;
		targ.enemy = attacker;
		if (!(targ.monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
	// if they *meant* to shoot us, then shoot back
	else if (attacker.enemy == targ) {
		if (targ.enemy.has_value() && targ.enemy->is_client())
			targ.oldenemy = targ.enemy;
		targ.enemy = attacker;
		if (!(targ.monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker.enemy.has_value() && attacker.enemy != targ) {
		if (targ.enemy.has_value() && targ.enemy->is_client())
			targ.oldenemy = targ.enemy;
		targ.enemy = attacker.enemy;
		if (!(targ.monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
}
#endif

void T_Damage(entity &targ, entity &inflictor, entity &attacker, vector dir, vector point, vector normal, int32_t damage, int32_t knockback, damage_flags dflags, means_of_death mod)
{
	int32_t	take;
	int32_t	save;
	int32_t	asave;
	int32_t	psave;
	temp_event	te_sparks;

	if (!targ.takedamage)
		return;
#ifdef SINGLE_PLAYER

	// easy mode takes half damage
	if (!skill && !deathmatch && targ.is_client())
	{
		damage = (int) (damage * 0.5f);
		if (!damage)
			damage = 1;
	}
#endif

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if (OnSameTeam(targ, attacker))
	{
		if (CheckTeamDamage(mod))
			damage = 0;
		else
			mod |= MOD_FRIENDLY_FIRE;
	}
	meansOfDeath = mod;

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	VectorNormalize(dir);

	// bonus damage for suprising a monster
	if (!(dflags & DAMAGE_RADIUS) && (targ.svflags & SVF_MONSTER) && attacker.is_client() && (!targ.enemy.has_value()) && (targ.health > 0))
		damage *= 2;

	if (targ.flags & FL_NO_KNOCKBACK)
		knockback = 0;
	// figure momentum add
	else if (knockback && !(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((targ.movetype != MOVETYPE_NONE) && (targ.movetype != MOVETYPE_BOUNCE) && (targ.movetype != MOVETYPE_PUSH) && (targ.movetype != MOVETYPE_STOP))
		{
			vector kvel;
			const float	calc_mass = max(50.f, (float) targ.mass);

			if (targ.is_client() && attacker == targ)
				kvel = dir * (1600.0f * knockback / calc_mass);  // the rocket jump hack...
			else
				kvel = dir * (500.0f * knockback / calc_mass);

			targ.velocity += kvel;
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ((targ.flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		take = 0;
		save = damage;
		SpawnDamage(te_sparks, point, normal);
	}

	// check for invincibility
	if (((targ.is_client() && targ.client->invincible_framenum > level.framenum) && !(dflags & DAMAGE_NO_PROTECTION))
#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
		|| (((targ.svflags & SVF_MONSTER) && targ.monsterinfo.invincible_framenum > level.framenum) && !(dflags & DAMAGE_NO_PROTECTION))
#endif
		)
	{
		if (targ.pain_debounce_framenum < level.framenum)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ.pain_debounce_framenum = level.framenum + 2 * BASE_FRAMERATE;
		}
		take = 0;
		save = damage;
	}

#ifdef CTF
	//team armor protect
	if (ctf.intVal && targ.is_client && attacker.is_client &&
		OnSameTeam(targ, attacker) && (dmflags.intVal & DF_ARMOR_PROTECT))
		psave = asave = 0;
	else
	{
#endif
		psave = CheckPowerArmor(targ, point, normal, take, dflags);
		take -= psave;

		asave = CheckArmor(targ, point, normal, take, te_sparks, dflags);
		take -= asave;
#ifdef CTF
	}
#endif

	//treat cheat/powerup savings the same as armor
	asave += save;

#ifdef GROUND_ZERO
	// this option will do damage both to the armor and person. originally for DPU rounds
	if (dflags & DAMAGE_DESTROY_ARMOR)
	{
		if (!(targ.flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) &&
			!(targ.is_client() && targ.client->invincible_framenum > level.framenum))
			take = damage;
	}
#endif

#ifdef CTF
	take = CTFApplyResistance(targ, take);

	CTFCheckHurtCarrier(targ, attacker);
#endif

	// do the damage
	if (take)
	{
		if ((targ.svflags & SVF_MONSTER) || targ.is_client())
		{
#if defined(THE_RECKONING) && defined(SINGLE_PLAYER)
			if (targ.classname == "monster_gekk")
				SpawnDamage(TE_GREENBLOOD, point, normal);
			else
#endif
#ifdef GROUND_ZERO
				if (targ.flags & FL_MECHANICAL)
					SpawnDamage(TE_ELECTRIC_SPARKS, point, normal);
				else if (mod == MOD_CHAINFIST)
					SpawnDamage(TE_MOREBLOOD, point, normal);
				else
#endif
					SpawnDamage(TE_BLOOD, point, normal);
		}
		else
			SpawnDamage(te_sparks, point, normal);

		targ.health -= take;

		if (targ.health <= 0)
		{
			if ((targ.svflags & SVF_MONSTER) || targ.is_client())
				targ.flags |= FL_NO_KNOCKBACK;
			Killed(targ, inflictor, attacker, take, point);
			return;
		}
	}

#ifdef SINGLE_PLAYER
	if (targ.svflags & SVF_MONSTER)
	{
		M_ReactToDamage(targ, attacker
#ifdef GROUND_ZERO
			, inflictor
#endif
		);

		if (!(targ.monsterinfo.aiflags & AI_DUCKED) && take)
		{
			if (targ.pain)
				targ.pain(targ, attacker, (float) knockback, take);

			// nightmare mode monsters don't go into pain frames often
			if ((int32_t) skill == 3)
				targ.pain_debounce_framenum = level.framenum + 5 * BASE_FRAMERATE;
		}
	}
	else
#endif
		if (targ.is_client())
		{
			if (!(targ.flags & FL_GODMODE) && take)
				targ.pain(targ, attacker, (float) knockback, take);
		}
		else if (take)
		{
			if (targ.pain)
				targ.pain(targ, attacker, (float) knockback, take);
		}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (targ.is_client())
	{
		targ.client->damage_parmor += psave;
		targ.client->damage_armor += asave;
		targ.client->damage_blood += take;
		targ.client->damage_knockback += knockback;
		targ.client->damage_from = point;
	}
}

void T_RadiusDamage(entity &inflictor, entity &attacker, float damage, entityref ignore, float radius, means_of_death mod)
{
	entityref	ent;

	while ((ent = findradius(ent, inflictor.s.origin, radius)).has_value())
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		float points = damage - 0.5f * VectorDistance(inflictor.s.origin, ent->s.origin + (0.5f * (ent->mins + ent->maxs)));
		if (ent == attacker)
			points = points * 0.5f;
		if (points > 0)
		{
			if (CanDamage(ent, inflictor))
			{
				vector dir = ent->s.origin - inflictor.s.origin;
				T_Damage(ent, inflictor, attacker, dir, inflictor.s.origin, vec3_origin, (int) points, (int) points, DAMAGE_RADIUS, mod);
			}
		}
	}
}