#include "entity.h"
#include "game.h"
#include "cmds.h"
#include "combat.h"
#include "util.h"
#include "lib/gi.h"
#include "items/armor.h"
#include "combat.h"
#ifdef GROUND_ZERO
#include "game/monster/medic.h"
#endif
#ifdef THE_RECKONING
#include "game/xatrix/monster/gekk.h"
#endif

bool CanDamage(entity &targ, entity &inflictor)
{
	// bmodels need special checking because their origin is 0,0,0
	if (targ.solid == SOLID_BSP)
	{
		trace tr = gi.traceline(inflictor.origin, targ.absbounds.center(), inflictor, MASK_SOLID);

		if (tr.fraction == 1.0f)
			return true;
		if (tr.ent == targ)
			return true;

		return false;
	}

	trace tr = gi.traceline(inflictor.origin, targ.origin, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	vector dest = targ.origin;
	dest.x += 15.0f;
	dest.y += 15.0f;
	tr = gi.traceline(inflictor.origin, dest, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	dest = targ.origin;
	dest.x += 15.0f;
	dest.y -= 15.0f;
	tr = gi.traceline(inflictor.origin, dest, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	dest = targ.origin;
	dest.x -= 15.0f;
	dest.y += 15.0f;
	tr = gi.traceline(inflictor.origin, dest, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	dest = targ.origin;
	dest.x -= 15.0f;
	dest.y -= 15.0f;
	tr = gi.traceline(inflictor.origin, dest, inflictor, MASK_SOLID);
	if (tr.fraction == 1.0f)
		return true;

	return false;
}

inline void SpawnDamage(temp_event type, vector origin, vector normal)
{
	gi.ConstructMessage(svc_temp_entity, type, origin, vecdir { normal }).multicast(origin, MULTICAST_PVS);
}

static inline int32_t CheckPowerArmor(entity &ent, vector point, vector normal, int damage, damage_style style)
{
	if (!damage)
		return 0;

	if (style.flags & (DAMAGE_NO_ARMOR
#ifdef GROUND_ZERO
		| DAMAGE_NO_POWER_ARMOR
#endif
		))
		return 0;

	gitem_id	power_armor_type;

	int32_t		power = 0;

	if (ent.is_client())
	{
		power_armor_type = PowerArmorType(ent);
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

	int32_t		damagePerCell;
	temp_event	pa_te_type;

	if (power_armor_type == ITEM_POWER_SCREEN)
	{
		vector	forward;

		// only works if damage point is in front
		AngleVectors(ent.angles, &forward, nullptr, nullptr);
		vector vec = point - ent.origin;
		VectorNormalize(vec);
		float dot = vec * forward;
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

	int32_t save = power * damagePerCell;

#ifdef GROUND_ZERO
	if (style.flags & DAMAGE_NO_REG_ARMOR)
		save /= 2;
#endif

	if (!save)
		return 0;

	save = min(damage, save);

	SpawnDamage(pa_te_type, point, normal);
	ent.powerarmor_framenum = level.framenum + 200ms;

	int32_t power_used = save / damagePerCell;

#ifdef GROUND_ZERO
	if (style.flags & DAMAGE_NO_REG_ARMOR)
		power_used *= 2;
#endif

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

static inline int32_t CheckArmor(entity &ent, vector point, vector normal, int32_t damage, damage_style style)
{
	if (!damage)
		return 0;

	if (!ent.is_client())
		return 0;

	if (style.flags & (DAMAGE_NO_ARMOR
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

	if (style.flags & DAMAGE_ENERGY)
		save = (int32_t) ceil(armor.energy_protection * damage);
	else
		save = (int32_t) ceil(armor.normal_protection * damage);

	if (save > ent.client->pers.inventory[index])
		save = ent.client->pers.inventory[index];

	if (!save)
		return 0;

	ent.client->pers.inventory[index] -= save;

	SpawnDamage(style.sparks, point, normal);

	return save;
}

#include "lib/info.h"

enum gender_id : uint8_t
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_NEUTRAL
};

inline gender_id GetGender(entity &ent)
{
	if (!ent.is_client())
		return GENDER_NEUTRAL;

	string info = Info_ValueForKey(ent.client->pers.userinfo, "gender");

	if (info[0] == 'f' || info[0] == 'F')
		return GENDER_FEMALE;
	else if (info[0] != 'm' && info[0] != 'm')
		return GENDER_NEUTRAL;

	return GENDER_MALE;
}

static void T_Obituary(entity &self, entity &attacker, means_of_death_ref mod) 
{
	gender_id gender = GetGender(self);

	constexpr stringlit their[] = {
		"his",
		"her",
		"their"
	};

	constexpr stringlit themself[] = {
		"himself",
		"herself",
		"themself"
	};

	bool self_kill = attacker == self;

	gi.bprintfmt(PRINT_MEDIUM, (self_kill || !mod.other_kill_fmt) ? (mod.self_kill_fmt ? mod.self_kill_fmt : MOD_DEFAULT.self_kill_fmt) : mod.other_kill_fmt,
		self.is_client() ? self.client->pers.netname : self.type->id, their[gender], themself[gender],
		attacker.is_client() ? attacker.client->pers.netname : attacker.type->id);

	if (self_kill)
	{
		if (self.is_client()
#ifdef SINGLE_PLAYER
			&& deathmatch
#endif
			)
			self.client->resp.score--;
	}
	else if (attacker.is_client()
#ifdef SINGLE_PLAYER
		&& deathmatch
		)
	{
#endif
		if (OnSameTeam(self, attacker))
			attacker.client->resp.score--;
		else
			attacker.client->resp.score++;
#ifdef SINGLE_PLAYER
	}
#endif
}

// temporary
static entity_type ET_MONSTER_CARRIER, ET_MONSTER_WIDOW, ET_MONSTER_WIDOW2, ET_MONSTER_TANK, ET_MONSTER_SUPERTANK, ET_MONSTER_MAKRON, ET_MONSTER_JORG;

static inline void Killed(entity &targ, entity &inflictor, entity &attacker, int32_t damage, vector point, means_of_death_ref mod)
{
	if (targ.health < -999)
		targ.health = -999;

#if defined(ROGUE_AI) && defined(SINGLE_PLAYER)
	if (targ.monsterinfo.aiflags & AI_MEDIC)
	{
		if (targ.enemy.has_value())  // god, I hope so
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
		if (targ.monsterinfo.commander.has_value() && targ.monsterinfo.commander->inuse)
		{
			entityref commander = targ.monsterinfo.commander;

			//ROGUE - free up slot for spawned monster if it's spawned
			if (((targ.monsterinfo.aiflags & AI_SPAWNED_CARRIER) && commander->type == ET_MONSTER_CARRIER) ||
				((targ.monsterinfo.aiflags & AI_SPAWNED_MEDIC_C) && commander->type == ET_MONSTER_MEDIC_COMMANDER))
				commander->monsterinfo.monster_slots++;
			else if (targ.monsterinfo.aiflags & AI_SPAWNED_WIDOW)
			{
				if (commander->type == ET_MONSTER_WIDOW || commander->type == ET_MONSTER_WIDOW2)
					if (commander->monsterinfo.monster_used)
						commander->monsterinfo.monster_used--;
			}
		}
#endif

		if (!(targ.monsterinfo.aiflags & AI_GOOD_GUY_MASK))
		{
			level.killed_monsters++;

			if (coop && attacker.is_client())
				attacker.client->resp.score++;

#ifndef ROGUE_AI
			// medics won't heal monsters that they kill themselves
			if (attacker.type == ET_MONSTER_MEDIC)
				targ.owner = attacker;
#endif
		}
	}
#endif

#ifdef SINGLE_PLAYER
	if ((targ.svflags & SVF_MONSTER) && (targ.deadflag != DEAD_DEAD))
	{
		targ.touch = nullptr;
		monster_death_use(targ);
	}
#endif

	if (targ.deadflag == DEAD_NO)
		T_Obituary(targ, attacker, mod);

	targ.die(targ, inflictor, attacker, damage, point);
}

void T_Damage(entity &targ, entity &inflictor, entity &attacker, vector dir, vector point, vector normal, int32_t damage, int32_t knockback, damage_style style, means_of_death_ref mod)
{
	int32_t	take;
	int32_t	save;
	int32_t	asave;
	int32_t	psave;

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
	if ((dmflags & DF_NO_FRIENDLY_FIRE) && !(style.flags & DAMAGE_IGNORE_FF) && OnSameTeam(targ, attacker))
		damage = 0;

	VectorNormalize(dir);

	// bonus damage for suprising a monster
	if (!(style.flags & DAMAGE_RADIUS) && (targ.svflags & SVF_MONSTER) && attacker.is_client() && (!targ.enemy.has_value()) && (targ.health > 0))
		damage *= 2;

	if (targ.flags & FL_NO_KNOCKBACK)
		knockback = 0;
	// figure momentum add
	else if (knockback && !(style.flags & DAMAGE_NO_KNOCKBACK))
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
	if ((targ.flags & FL_GODMODE) && !(style.flags & DAMAGE_NO_PROTECTION))
	{
		take = 0;
		save = damage;
		SpawnDamage(style.sparks, point, normal);
	}

	// check for invincibility
	if (!(style.flags & DAMAGE_NO_PROTECTION) && ((targ.is_client() && targ.client->invincible_framenum > level.framenum)
#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
		|| ((targ.svflags & SVF_MONSTER) && targ.monsterinfo.invincible_framenum > level.framenum))
#endif
		)
	{
		if (targ.pain_debounce_framenum < level.framenum)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"));
			targ.pain_debounce_framenum = level.framenum + 2s;
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
		psave = CheckPowerArmor(targ, point, normal, take, style);
		take -= psave;

		asave = CheckArmor(targ, point, normal, take, style);
		take -= asave;
#ifdef CTF
	}
#endif

	//treat cheat/powerup savings the same as armor
	asave += save;

#ifdef GROUND_ZERO
	// this option will do damage both to the armor and person. originally for DPU rounds
	if (style.flags & DAMAGE_DESTROY_ARMOR)
	{
		if (!(targ.flags & FL_GODMODE) && !(style.flags & DAMAGE_NO_PROTECTION) &&
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
		temp_event type;

		if ((targ.svflags & SVF_MONSTER) || targ.is_client())
		{
			switch (targ.bleed_style)
			{
			case BLEED_GREEN:
				type = TE_GREENBLOOD;
				break;
			case BLEED_MECHANICAL:
				type = TE_ELECTRIC_SPARKS;
				break;
			default:
				type = style.blood;
				break;
			}
		}
		else
			type = style.sparks;

		SpawnDamage(type, point, normal);

		targ.health -= take;

		if (targ.health <= 0)
		{
			if (style.flags & DAMAGE_INSTANT_GIB)
			{
				take = 400;
				targ.health = targ.gib_health ? targ.gib_health : -take;
			}

			if ((targ.svflags & SVF_MONSTER) || targ.is_client())
				targ.flags |= FL_NO_KNOCKBACK;

			Killed(targ, inflictor, attacker, take, point, mod);
			return;
		}
	}

	if ((!targ.is_client() || !(targ.flags & FL_GODMODE)) && take && targ.pain)
		targ.pain(targ, attacker, (float) knockback, take);

#ifdef SINGLE_PLAYER
	if (targ.svflags & SVF_MONSTER)
		M_ReactToDamage(targ, attacker, inflictor, knockback, take);
#endif

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

void T_RadiusDamage(entity &inflictor, entity &attacker, float damage, entityref ignore, float radius, means_of_death_ref mod, damage_style style)
{
	for (entity &ent : G_IterateRadius(inflictor.origin, radius))
	{
		if (ent == ignore)
			continue;
		if (!ent.takedamage)
			continue;

		float points = damage - 0.5f * VectorDistance(inflictor.origin, ent.origin + ent.bounds.center());

		if (!(style.flags & DAMAGE_NO_RADIUS_HALF) && ent == attacker)
			points = points * 0.5f;

		if (points > 0 && CanDamage(ent, inflictor))
		{
			vector dir = ent.origin - inflictor.origin;
			T_Damage(ent, inflictor, attacker, dir, inflictor.origin, vec3_origin, (int32_t) points, (int32_t) points, style, mod);
		}
	}
}