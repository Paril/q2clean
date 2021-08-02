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

constexpr float NUKE_DELAY			= 4.f;
constexpr float NUKE_TIME_TO_LIVE	= 6.f;
constexpr float NUKE_RADIUS			= 512.f;
constexpr int32_t NUKE_DAMAGE		= 400;
constexpr float NUKE_QUAKE_TIME		= 3.f;
constexpr float NUKE_QUAKE_STRENGTH	= 100.f;

static entity_type ET_NUKE("nuke", ET_GRENADE);

static void Nuke_Quake(entity &self)
{
	if (self.last_move_framenum < level.framenum)
	{
		gi.positioned_sound (self.s.origin, self, CHAN_AUTO, self.noise_index, 0.75f, ATTN_NONE, 0);
		self.last_move_framenum = level.framenum + (int)(0.5 * BASE_FRAMERATE);
	}

	for (uint32_t i = 1; i < num_entities; i++)
	{
		entity &e = itoe(i);

		if (!e.inuse)
			continue;
		if (!e.is_client())
			continue;
		if (e.groundentity == null_entity)
			continue;

		e.groundentity = null_entity;
		e.velocity[0] += random(-150.f, 150.f);
		e.velocity[1] += random(-150.f, 150.f);
		e.velocity[2] = self.speed * (100.0f / e.mass);
	}

	if (level.framenum < self.timestamp)
		self.nextthink = level.framenum + 1;
	else
		G_FreeEdict (self);
}

static REGISTER_SAVABLE_FUNCTION(Nuke_Quake);

/*
============
T_RadiusNukeDamage

Like T_RadiusDamage, but ignores walls (skips CanDamage check, among others)
// up to KILLZONE radius, do 10,000 points
// after that, do damage linearly out to KILLZONE2 radius
============
*/
static inline void T_RadiusNukeDamage(entity &inflictor, entity &attacker, float damage, entityref ignore, float radius, means_of_death mod)
{
	float killzone = radius;
	float killzone2 = radius * 2.f;

	entityref ent;

	while ((ent = findradius(ent, inflictor.s.origin, killzone2)).has_value())
	{
// ignore nobody
		if (ent == ignore)
			continue;
		if (!ent->inuse)
			continue;
		if (!ent->takedamage)
			continue;
		if (!(ent->is_client() || (ent->svflags & SVF_MONSTER) || (ent->flags & FL_DAMAGEABLE)))
			continue;

		vector v = ent->s.origin + ent->bounds.center();
		v = inflictor.s.origin - v;

		float len = VectorLength(v);
		float points;
		
		if (len <= killzone)
		{
			if (ent->is_client())
				ent->flags |= FL_NOGIB;
			points = 10000.f;
		}
		else if (len <= killzone2)
			points = (damage / killzone) * (killzone2 - len);
		else
			points = 0;

		if (points > 0)
		{
			if (ent->is_client())
				ent->client->nuke_framenum = level.framenum + 20;
			vector dir = ent->s.origin - inflictor.s.origin;
			T_Damage (ent, inflictor, attacker, dir, inflictor.s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
		}
	}

	ent = itoe(1); // skip the worldspawn

	// cycle through players
	while (ent.has_value())
	{
		if (ent->is_client() && (ent->client->nuke_framenum != level.framenum + 20) && ent->inuse)
		{
			trace tr = gi.traceline(inflictor.s.origin, ent->s.origin, inflictor, MASK_SOLID);

			if (tr.fraction == 1.0)
				ent->client->nuke_framenum = level.framenum + 20;
			else
			{
				float dist = VectorDistance(ent->s.origin, inflictor.s.origin);
				
				if (dist < 2048)
					ent->client->nuke_framenum = max(ent->client->nuke_framenum, level.framenum + 15);
				else
					ent->client->nuke_framenum = max(ent->client->nuke_framenum, level.framenum + 10);
			}

			ent = next_ent(ent);
		}
		else
			ent = null_entity;
	}
}

static void Nuke_Explode(entity &ent)
{
#if defined(SINGLE_PLAYER)
	if (ent.teammaster->is_client())
		PlayerNoise(ent.teammaster, ent.s.origin, PNOISE_IMPACT);

#endif
	T_RadiusNukeDamage(ent, ent.teammaster, ent.dmg, ent, ent.dmg_radius, MOD_NUKE);

	if (ent.dmg > NUKE_DAMAGE)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

	gi.sound (ent, CHAN_NO_PHS_ADD | CHAN_VOICE, gi.soundindex ("weapons/grenlx1a.wav"), 1, ATTN_NONE, 0);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1_BIG);
	gi.WritePosition (ent.s.origin);
	gi.multicast (ent.s.origin, MULTICAST_PVS);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_NUKEBLAST);
	gi.WritePosition (ent.s.origin);
	gi.multicast (ent.s.origin, MULTICAST_ALL);

	// become a quake
	ent.svflags |= SVF_NOCLIENT;
	ent.noise_index = gi.soundindex ("world/rumble.wav");
	ent.think = SAVABLE(Nuke_Quake);
	ent.speed = NUKE_QUAKE_STRENGTH;
	ent.timestamp = level.framenum + (gtime)(NUKE_QUAKE_TIME * BASE_FRAMERATE);
	ent.nextthink = level.framenum + 1;
	ent.last_move_framenum = 0;
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

static REGISTER_SAVABLE_FUNCTION(nuke_die);

static void Nuke_Think(entity &ent);

DECLARE_SAVABLE_FUNCTION(Nuke_Think);

static void Nuke_Think(entity &ent)
{
	sound_attn attenuation = ATTN_STATIC;
	muzzleflash	muzzleflash = MZ_NUKE1;

	int32_t damage_multiplier = ent.dmg / NUKE_DAMAGE;
	switch (damage_multiplier)
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

	if (ent.wait < level.framenum)
		Nuke_Explode(ent);
	else if (level.framenum >= (ent.wait - (gtime)(NUKE_TIME_TO_LIVE * BASE_FRAMERATE)))
	{
		ent.s.frame++;

		if (ent.s.frame > 11)
			ent.s.frame = 6;

		if (gi.pointcontents(ent.s.origin) & (CONTENTS_SLIME|CONTENTS_LAVA))
		{
			Nuke_Explode (ent);
			return;
		}

		ent.think = SAVABLE(Nuke_Think);
		ent.nextthink = level.framenum + 1;
		ent.health = 1;
		ent.owner = 0;

		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent.s.number);
		gi.WriteByte (muzzleflash);
		gi.multicast (ent.s.origin, MULTICAST_PVS);

		if (ent.timestamp <= level.framenum)
		{
			if ((ent.wait - level.framenum) <= (NUKE_TIME_TO_LIVE / 2.0))
			{
				gi.sound (ent, CHAN_NO_PHS_ADD | CHAN_VOICE, gi.soundindex ("weapons/nukewarn2.wav"), 1, attenuation, 0);
				ent.timestamp = level.framenum + (gtime)(0.3f * BASE_FRAMERATE);
			}
			else
			{
				gi.sound (ent, CHAN_NO_PHS_ADD | CHAN_VOICE, gi.soundindex ("weapons/nukewarn2.wav"), 1, attenuation, 0);
				ent.timestamp = level.framenum + (gtime)(0.5f * BASE_FRAMERATE);
			}
		}
	}
	else
	{
		if (ent.timestamp <= level.framenum)
		{
			gi.sound (ent, CHAN_NO_PHS_ADD | CHAN_VOICE, gi.soundindex ("weapons/nukewarn2.wav"), 1, attenuation, 0);
			ent.timestamp = level.framenum + BASE_FRAMERATE;
		}

		ent.nextthink = level.framenum + 1;
	}
}

static REGISTER_SAVABLE_FUNCTION(Nuke_Think);

static void nuke_bounce(entity &ent, entity &, vector, const surface &)
{
	if (random() > 0.5)
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
}

static REGISTER_SAVABLE_FUNCTION(nuke_bounce);

void fire_nuke(entity &self, vector start, vector aimdir, int32_t speed)
{
	P_DamageModifier (self);

	vector dir = vectoangles (aimdir);
	vector right, up;
	AngleVectors (dir, nullptr, &right, &up);

	entity &nuke = G_Spawn();
	nuke.s.origin = start;
	nuke.velocity = aimdir * speed;
	nuke.velocity += random(190.f, 210.f) * up;
	nuke.velocity += random(-10.f, 10.f) * right;
	nuke.movetype = MOVETYPE_BOUNCE;
	nuke.clipmask = MASK_SHOT;
	nuke.solid = SOLID_BBOX;
	nuke.s.effects |= EF_GRENADE;
	nuke.s.renderfx |= RF_IR_VISIBLE;
	nuke.bounds = {
		.mins = { -8, -8, 0 },
		.maxs = { 8, 8, 16 }
	};
	nuke.s.modelindex = gi.modelindex ("models/weapons/g_nuke/tris.md2");
	nuke.owner = self;
	nuke.teammaster = self;
	nuke.nextthink = level.framenum + 1;
	nuke.wait = level.framenum + ((NUKE_DELAY + NUKE_TIME_TO_LIVE) * BASE_FRAMERATE);
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