#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/util.h"
#include "game/combat.h"
#ifdef SINGLE_PLAYER
#include "game/weaponry.h"
#include "game/ballistics.h"
#endif
#include "game/ballistics/grenade.h"
#include "nuke.h"

constexpr means_of_death MOD_NUKE { .self_kill_fmt = "{0}'s destruction was mutually assured.\n", .other_kill_fmt = "{0} was nuked by {3}'s antimatter bomb.\n" };

constexpr gtime NUKE_DELAY			= 4s;
constexpr gtime NUKE_TIME_TO_LIVE	= 6s;
constexpr float NUKE_RADIUS			= 512.f;
constexpr int32_t NUKE_DAMAGE		= 400;
constexpr gtime NUKE_QUAKE_TIME		= 3s;
constexpr float NUKE_QUAKE_STRENGTH	= 100.f;

static entity_type ET_NUKE("nuke", ET_GRENADE);

static void Nuke_Quake(entity &self)
{
	if (self.last_move_time < level.time)
	{
		gi.positioned_sound(self.origin, self, self.noise_index, 0.75f, ATTN_NONE);
		self.last_move_time = level.time + 500ms;
	}

	for (uint32_t i = 1; i < num_entities; i++)
	{
		entity &e = itoe(i);

		if (!e.inuse)
			continue;
		if (!e.is_client)
			continue;
		if (e.groundentity == null_entity)
			continue;

		e.groundentity = null_entity;
		e.velocity[0] += crandom(150.f);
		e.velocity[1] += crandom(150.f);
		e.velocity[2] = self.speed * (100.0f / e.mass);
	}

	if (level.time < self.timestamp)
		self.nextthink = level.time + 1_hz;
	else
		G_FreeEdict (self);
}

REGISTER_STATIC_SAVABLE(Nuke_Quake);

/*
============
T_RadiusNukeDamage

Like T_RadiusDamage, but ignores walls (skips CanDamage check, among others)
// up to KILLZONE radius, do 10,000 points
// after that, do damage linearly out to KILLZONE2 radius
============
*/
inline void T_RadiusNukeDamage(entity &inflictor, entity &attacker, float damage, entityref ignore, float radius, means_of_death_ref mod)
{
	float killzone = radius;
	float killzone2 = radius * 2.f;

	for (entity &ent : G_IterateRadius(inflictor.origin, killzone2))
	{
		if (ent == ignore)
			continue;
		if (!ent.inuse)
			continue;
		if (!ent.takedamage)
			continue;
		if (!(ent.is_client || (ent.svflags & SVF_MONSTER) || (ent.flags & FL_DAMAGEABLE)))
			continue;

		vector v = ent.origin + ent.bounds.center();
		v = inflictor.origin - v;

		float len = VectorLength(v);
		float points;
		
		if (len <= killzone)
			points = 10000.f;
		else if (len <= killzone2)
			points = (damage / killzone) * (killzone2 - len);
		else
			points = 0;

		if (points > 0)
		{
			if (ent.is_client)
				ent.client.nuke_time = level.time + 2s;

			vector dir = ent.origin - inflictor.origin;
			if (ent.is_client)
				ent.flags |= FL_NOGIB;
			T_Damage (ent, inflictor, attacker, dir, inflictor.origin, vec3_origin, (int)points, (int)points, { DAMAGE_RADIUS }, mod);
			if (ent.is_client)
				ent.flags &= ~FL_NOGIB;
		}
	}

	// cycle through players
	for (auto &ent : entity_range(1, game.maxclients))
	{
		if (ent.inuse && ent.client.nuke_time != level.time + 2s)
		{
			trace tr = gi.traceline(inflictor.origin, ent.origin, inflictor, MASK_SOLID);

			if (tr.fraction == 1.0)
				ent.client.nuke_time = level.time + 2s;
			else
			{
				float dist = VectorDistance(ent.origin, inflictor.origin);
				ent.client.nuke_time = max(ent.client.nuke_time, level.time + (dist < 2048 ? 1500ms : 1s));
			}
		}
	}
}

static void Nuke_Explode(entity &ent)
{
#if defined(SINGLE_PLAYER)
	if (ent.teammaster->is_client)
		PlayerNoise(ent.teammaster, ent.origin, PNOISE_IMPACT);

#endif
	T_RadiusNukeDamage(ent, ent.teammaster, ent.dmg, ent, ent.dmg_radius, MOD_NUKE);

	if (ent.dmg > NUKE_DAMAGE)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"));

	gi.sound(ent, CHAN_VOICE, gi.soundindex ("weapons/grenlx1a.wav"), ATTN_NONE);

	gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1_BIG, ent.origin).multicast(ent.origin, MULTICAST_PVS);

	gi.ConstructMessage(svc_temp_entity, TE_NUKEBLAST, ent.origin).multicast(ent.origin, MULTICAST_PVS);

	// become a quake
	ent.svflags |= SVF_NOCLIENT;
	ent.noise_index = gi.soundindex ("world/rumble.wav");
	ent.think = SAVABLE(Nuke_Quake);
	ent.speed = NUKE_QUAKE_STRENGTH;
	ent.timestamp = level.time + NUKE_QUAKE_TIME;
	ent.nextthink = level.time + 1_hz;
	ent.last_move_time = gtime::zero();
}

static void nuke_die(entity &self, entity &, entity &attacker, int32_t, vector)
{
	self.takedamage = false;

	if (attacker.type == ET_NUKE)
	{
		G_FreeEdict (self);	
		return;
	}

	Nuke_Explode(self);
}

REGISTER_STATIC_SAVABLE(nuke_die);

static void Nuke_Think(entity &ent);

REGISTER_STATIC_SAVABLE(Nuke_Think);

static void Nuke_Think(entity &ent)
{
	sound_attn attenuation = ATTN_STATIC;
	muzzleflash	muzzleflash = MZ_NUKE1;

	int32_t dmg_multiplier = ent.dmg / NUKE_DAMAGE;
	switch (dmg_multiplier)
	{
	case 1:
		break;
	case 2:
		attenuation = ATTN_IDLE;
		muzzleflash = MZ_NUKE2;
		break;
	case 4:
		attenuation = ATTN_NORM;
		muzzleflash = MZ_NUKE4;
		break;
	case 8:
		attenuation = ATTN_NONE;
		muzzleflash = MZ_NUKE8;
		break;
	}

	if (ent.wait < level.time)
		Nuke_Explode(ent);
	else if (level.time >= (ent.wait - NUKE_TIME_TO_LIVE))
	{
		ent.frame++;

		if (ent.frame > 11)
			ent.frame = 6;

		if (gi.pointcontents(ent.origin) & (CONTENTS_SLIME|CONTENTS_LAVA))
		{
			Nuke_Explode (ent);
			return;
		}

		ent.think = SAVABLE(Nuke_Think);
		ent.nextthink = level.time + 100ms;
		ent.health = 1;
		ent.owner = 0;

		gi.ConstructMessage(svc_muzzleflash, ent, muzzleflash).multicast(ent.origin, MULTICAST_PVS);

		if (ent.timestamp <= level.time)
		{
			gtime next_tick;

			if ((ent.wait - level.time) <= (NUKE_TIME_TO_LIVE / 2.0))
				next_tick = 300ms;
			else
				next_tick = 500ms;

			ent.timestamp = level.time + next_tick;

			gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/nukewarn2.wav"), attenuation);
		}
	}
	else
	{
		if (ent.timestamp <= level.time)
		{
			gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/nukewarn2.wav"), attenuation);
			ent.timestamp = level.time + 1s;
		}

		ent.nextthink = level.time + 100ms;
	}
}

static void nuke_bounce(entity &ent, entity &, vector, const surface &)
{
	if (random() > 0.5)
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb1a.wav"));
	else
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb2a.wav"));
}

REGISTER_STATIC_SAVABLE(nuke_bounce);

void fire_nuke(entity &self, vector start, vector aimdir, int32_t speed)
{
	P_DamageModifier (self);

	vector dir = vectoangles (aimdir);
	vector right, up;
	AngleVectors (dir, nullptr, &right, &up);

	entity &nuke = G_Spawn();
	nuke.origin = start;
	nuke.velocity = aimdir * speed;
	nuke.velocity += random(190.f, 210.f) * up;
	nuke.velocity += crandom(10.f) * right;
	nuke.movetype = MOVETYPE_BOUNCE;
	nuke.clipmask = MASK_SHOT;
	nuke.solid = SOLID_BBOX;
	nuke.effects |= EF_GRENADE;
	nuke.renderfx |= RF_IR_VISIBLE;
	nuke.bounds = {
		.mins = { -8, -8, 0 },
		.maxs = { 8, 8, 16 }
	};
	nuke.modelindex = gi.modelindex ("models/weapons/g_nuke/tris.md2");
	nuke.owner = self;
	nuke.teammaster = self;
	nuke.nextthink = level.time + 1_hz;
	nuke.wait = level.time + NUKE_DELAY + NUKE_TIME_TO_LIVE;
	nuke.think = SAVABLE(Nuke_Think);
	nuke.touch = SAVABLE(nuke_bounce);

	nuke.health = 10000;
	nuke.takedamage = true;
	nuke.flags |= FL_DAMAGEABLE;
	nuke.dmg = NUKE_DAMAGE * damage_multiplier;

	if (damage_multiplier == 1)
		nuke.dmg_radius = NUKE_RADIUS;
	else
		nuke.dmg_radius = NUKE_RADIUS + NUKE_RADIUS * (0.25f * damage_multiplier);

	nuke.type = ET_NUKE;
	nuke.die = SAVABLE(nuke_die);

	gi.linkentity (nuke);
}
#endif