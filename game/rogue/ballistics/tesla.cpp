#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/util.h"
#include "game/combat.h"
#ifdef SINGLE_PLAYER
#include "game/weaponry.h"
#include "game/ballistics.h"
#endif
#include "game/player.h"
#include "game/misc.h"
#include "game/ballistics/grenade.h"
#include "tesla.h"

constexpr means_of_death MOD_TESLA { .self_kill_fmt = "{0} zapped {2}.\n", .other_kill_fmt = "{0} was enlightened by {3}'s tesla mine.\n" };

constexpr float TESLA_TIME_TO_LIVE				= 30.f;
constexpr int32_t TESLA_DAMAGE					= 3;		// 3
constexpr int32_t TESLA_KNOCKBACK				= 8;
constexpr int32_t TESLA_ACTIVATE_TIME			= 3;
constexpr int32_t TESLA_EXPLOSION_DAMAGE_MULT	= 50;		// this is the amount the damage is multiplied by for underwater explosions
constexpr float TESLA_EXPLOSION_RADIUS			= 200.f;

entity_type ET_TESLA("tesla", ET_GRENADE);

static void tesla_remove(entity &self)
{
	self.takedamage = false;
	if(self.teamchain.has_value())
	{
		entityref cur = self.teamchain;
		while(cur.has_value())
		{
			entityref next = cur->teamchain;
			G_FreeEdict(cur);
			cur = next;
		}
	}
	else if (self.air_finished_framenum)
		gi.dprint("tesla without a field!\n");

	self.owner = self.teammaster;	// Going away, set the owner correctly.
	// PGM - grenade explode does damage to self.enemy
	self.enemy = 0;

	// play quad sound if quadded and an underwater explosion
	if ((self.dmg_radius) && (self.dmg > (TESLA_DAMAGE*TESLA_EXPLOSION_DAMAGE_MULT)))
		gi.sound(self, CHAN_ITEM, gi.soundindex("items/damage3.wav"));

	Grenade_Explode(self);
}

static void tesla_die(entity &self, entity &, entity &, int32_t, vector)
{
	tesla_remove(self);
}

REGISTER_STATIC_SAVABLE(tesla_die);

static void tesla_blow(entity &self)
{
	self.dmg *= TESLA_EXPLOSION_DAMAGE_MULT;
	self.dmg_radius = TESLA_EXPLOSION_RADIUS;
	tesla_remove(self);
}

static void tesla_think_active(entity &self);

REGISTER_STATIC_SAVABLE(tesla_think_active);

static void tesla_think_active(entity &self)
{
	if (level.framenum > self.air_finished_framenum)
	{
		tesla_remove(self);
		return;
	}

	vector start = self.origin;
	start[2] += 16;

	dynarray<entityref> entities = gi.BoxEdicts(self.teamchain->absbounds, AREA_SOLID);

	for (auto &hit : entities)
	{
		// if the tesla died while zapping things, stop zapping.
		if(!self.inuse)
			break;

		if (!hit->inuse)
			continue;
		if (hit == self)
			continue;
		if (hit->health < 1)
			continue;

#ifdef SINGLE_PLAYER
		// don't hit clients in single-player or coop
		if (hit->is_client())
			if (coop || !deathmatch)
				continue;
#endif
		if (!(hit->svflags & SVF_MONSTER) && !(hit->flags & FL_DAMAGEABLE) && !hit->is_client())
			continue;
	
		trace tr = gi.traceline(start, hit->origin, self, MASK_SHOT);

		if (tr.fraction == 1 || tr.ent == hit)
		{
			vector dir = hit->origin - start;
			
			// PMM - play quad sound if it's above the "normal" damage
			if (self.dmg > TESLA_DAMAGE)
				gi.sound(self, CHAN_ITEM, gi.soundindex("items/damage3.wav"));

#ifdef SINGLE_PLAYER
			// PGM - don't do knockback to walking monsters
			if ((hit->svflags & SVF_MONSTER) && !(hit->flags & (FL_FLY | FL_SWIM)))
				T_Damage (hit, self, self.teammaster, dir, tr.endpos, tr.normal,
					self.dmg, 0, { DAMAGE_NONE }, MOD_TESLA);
			else
#endif
				T_Damage (hit, self, self.teammaster, dir, tr.endpos, tr.normal,
					self.dmg, TESLA_KNOCKBACK, { DAMAGE_NONE }, MOD_TESLA);

			gi.ConstructMessage(svc_temp_entity, TE_LIGHTNING, hit, self, tr.endpos, start).multicast(start, MULTICAST_PVS);
		}
	}

	if(self.inuse)
	{
		self.think = SAVABLE(tesla_think_active);
		self.nextthink = level.framenum + 1;
	}
}

static void tesla_activate(entity &self)
{
	if (gi.pointcontents (self.origin) & (CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_WATER))
	{
		tesla_blow (self);
		return;
	}
	
#ifdef SINGLE_PLAYER
	// only check for spawn points in deathmatch
	if (deathmatch)
	{
#endif
		entityref search;
		while ((search = findradius(search, self.origin, 1.5f * TESLA_DAMAGE_RADIUS)).has_value())
		{
			// if it's a monster or player with health > 0
			// or it's a deathmatch start point
			// and we can see it
			// blow up
			if ((search->type == ET_INFO_PLAYER_DEATHMATCH
				|| search->type == ET_INFO_PLAYER_START
#ifdef SINGLE_PLAYER
				|| search->type == ET_INFO_PLAYER_COOP
#endif
				|| search->type == ET_MISC_TELEPORTER_DEST)
				&& visible(search, self))
			{
				tesla_remove (self);
				return;
			}
		}
#ifdef SINGLE_PLAYER
	}
#endif

	entity &trigger = G_Spawn();
	trigger.origin = self.origin;
	trigger.bounds = {
		.mins = { -TESLA_DAMAGE_RADIUS, -TESLA_DAMAGE_RADIUS, self.bounds.mins[2] },
		.maxs = { TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS }
	};
	trigger.movetype = MOVETYPE_NONE;
	trigger.solid = SOLID_TRIGGER;
	trigger.owner = self;
	trigger.touch = nullptr;
	// doesn't need to be marked as a teamslave since the move code for bounce looks for teamchains
	gi.linkentity (trigger);

	self.angles = vec3_origin;
#ifdef SINGLE_PLAYER
	// clear the owner if in deathmatch
	if (deathmatch)
#endif
		self.owner = 0;
	self.teamchain = trigger;
	self.think = SAVABLE(tesla_think_active);
	self.nextthink = level.framenum + 1;
	self.air_finished_framenum = level.framenum + (gtime)(TESLA_TIME_TO_LIVE * BASE_FRAMERATE);
}

REGISTER_STATIC_SAVABLE(tesla_activate);

static void tesla_think(entity &ent);

REGISTER_STATIC_SAVABLE(tesla_think);

static void tesla_think(entity &ent)
{
	if (gi.pointcontents(ent.origin) & (CONTENTS_SLIME|CONTENTS_LAVA))
	{
		tesla_remove(ent);
		return;
	}
	
	ent.angles = vec3_origin;

	if (!ent.frame)
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/teslaopen.wav")); 

	ent.frame++;

	if(ent.frame > 14)
	{
		ent.frame = 14;
		ent.think = SAVABLE(tesla_activate);
		ent.nextthink = level.framenum + 1;
		return;
	}

	if (ent.frame > 9)
	{
		if (ent.frame == 10)
#if defined(SINGLE_PLAYER)
		{
			if (ent.owner.has_value() && ent.owner->is_client())
				PlayerNoise(ent.owner, ent.origin, PNOISE_WEAPON);		// PGM
#endif
			ent.skinnum = 1;
#if defined(SINGLE_PLAYER)
		}
#endif
		else if(ent.frame == 12)
			ent.skinnum = 2;
		else if(ent.frame == 14)
			ent.skinnum = 3;
	}
	ent.think = SAVABLE(tesla_think);
	ent.nextthink = level.framenum + 1;
}

static void tesla_lava(entity &ent, entity &, vector normal, const surface &)
{
	if (normal)
	{
		vector land_point = ent.origin + (normal * -20.0f);

		if (gi.pointcontents(land_point) & (CONTENTS_SLIME | CONTENTS_LAVA))
		{
			tesla_blow (ent);
			return;
		}
	}

	if (random() > 0.5)
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb1a.wav"));
	else
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb2a.wav"));
}

REGISTER_STATIC_SAVABLE(tesla_lava);

void fire_tesla(entity &self, vector start, vector aimdir, int32_t dmg_multiplier, int32_t speed)
{
	vector dir = vectoangles (aimdir);

	vector right, up;
	AngleVectors (dir, nullptr, &right, &up);

	entity &tesla = G_Spawn();
	tesla.origin = start;
	tesla.velocity = aimdir * speed;
	tesla.velocity += random(190.f, 210.f) * up;
	tesla.velocity += random(-10.f, 10.f) * right;
	tesla.movetype = MOVETYPE_BOUNCE;
	tesla.solid = SOLID_BBOX;
	tesla.effects |= EF_GRENADE;
	tesla.renderfx |= RF_IR_VISIBLE;
	tesla.bounds = {
		.mins = { -12, -12, 0 },
		.maxs = { 12, 12, 20 }
	};
	tesla.modelindex = gi.modelindex ("models/weapons/g_tesla/tris.md2");
	
	tesla.owner = self;		// PGM - we don't want it owned by self YET.
	tesla.teammaster = self;

	tesla.wait = (float)(level.framenum + (gtime)(TESLA_TIME_TO_LIVE * BASE_FRAMERATE));
	tesla.think = SAVABLE(tesla_think);
	tesla.nextthink = level.framenum + (TESLA_ACTIVATE_TIME * BASE_FRAMERATE);

	// blow up on contact with lava & slime code
	tesla.touch = SAVABLE(tesla_lava);

#ifdef SINGLE_PLAYER
	if (deathmatch)
		tesla.health = 30;
	else
#endif
		tesla.health = 20;

	tesla.takedamage = true;
	tesla.die = SAVABLE(tesla_die);
	tesla.dmg = TESLA_DAMAGE * dmg_multiplier;
	tesla.type = ET_TESLA;
	tesla.flags |= FL_DAMAGEABLE;
	tesla.clipmask = MASK_SHOT|CONTENTS_SLIME|CONTENTS_LAVA;
	tesla.bleed_style = BLEED_MECHANICAL;

	gi.linkentity (tesla);
}
#endif