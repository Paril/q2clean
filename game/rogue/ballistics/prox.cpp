#include "config.h"

#ifdef GROUND_ZERO
#include "game/combat.h"
#include "game/entity.h"
#include "game/util.h"
#ifdef SINGLE_PLAYER
#include "game/weaponry.h"
#include "game/ballistics.h"
#endif
#include "game/ballistics/grenade.h"
#include "game/misc.h"
#include "game/player.h"
#include "prox.h"

constexpr means_of_death MOD_PROX { .self_kill_fmt = "{0} forgot about {1} own mine.\n", .other_kill_fmt = "{0} got too close to {3}'s proximity mine.\n" };

constexpr gtime PROX_TIME_TO_LIVE	= 45s;		// 45, 30, 15, 10
constexpr gtime PROX_TIME_DELAY		= 500ms;
constexpr bbox PROX_BOUND_SIZE		= bbox::sized(96.f);
constexpr float PROX_DAMAGE_RADIUS	= 192.f;
constexpr int32_t PROX_HEALTH		= 20;
constexpr int32_t PROX_DAMAGE		= 90;

static entity_type ET_PROX("prox", ET_GRENADE);

static void Prox_Explode(entity &ent)
{
	//PMM - changed teammaster to "mover" .. owner of the field is the prox
	if (ent.teamchain.has_value() && ent.teamchain->owner == ent)
		G_FreeEdict(ent.teamchain);

	entityref cowner = ent;

	if (ent.teammaster.has_value())
#if defined(SINGLE_PLAYER)
	{
#endif
		cowner = ent.teammaster;
#if defined(SINGLE_PLAYER)
		PlayerNoise(cowner, ent.origin, PNOISE_IMPACT);
	}
#endif

	// play quad sound if appopriate
	if (ent.dmg > PROX_DAMAGE)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"));

	ent.takedamage = false;
	T_RadiusDamage(ent, cowner, ent.dmg, ent, PROX_DAMAGE_RADIUS, MOD_PROX);

	vector origin = ent.origin + (ent.velocity * -0.02f);

	gi.ConstructMessage(svc_temp_entity, ent.groundentity.has_value() ? TE_GRENADE_EXPLOSION : TE_ROCKET_EXPLOSION, origin).multicast(ent.origin, MULTICAST_PVS);

	G_FreeEdict (ent);
}

REGISTER_STATIC_SAVABLE(Prox_Explode);

static void prox_die(entity &self, entity &inflictor, entity &, int32_t, vector)
{
	// if set off by another prox, delay a little (chained explosions)
	self.takedamage = false;

	if (inflictor.type != ET_PROX)
		Prox_Explode(self);
	else
	{
		self.think = SAVABLE(Prox_Explode);
		self.nextthink = level.framenum + 100ms;
	}
}

REGISTER_STATIC_SAVABLE(prox_die);

static void Prox_Field_Touch(entity &ent, entity &other, vector, const surface &)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
		return;

	// trigger the prox mine if it's still there, and still mine.
	entityref prox = ent.owner;

	if (other == prox) // don't set self off
		return;

	if (prox->think == Prox_Explode) // we're set to blow!
		return;

	if (prox->teamchain == ent)
	{
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/proxwarn.wav"));
		prox->think = SAVABLE(Prox_Explode);
		prox->nextthink = level.framenum + PROX_TIME_DELAY;
		return;
	}

	ent.solid = SOLID_NOT;
	G_FreeEdict(ent);
}

REGISTER_STATIC_SAVABLE(Prox_Field_Touch);

static void prox_seek(entity &ent);

REGISTER_STATIC_SAVABLE(prox_seek);

static void prox_seek(entity &ent)
{
	if (level.framenum > ent.wait)
		Prox_Explode(ent);
	else
	{
		ent.frame++;
		if (ent.frame > 13)
			ent.frame = 9;
		ent.think = SAVABLE(prox_seek);
		ent.nextthink = level.framenum + 100ms;
	}
}

static void prox_open(entity &ent);

REGISTER_STATIC_SAVABLE(prox_open);

static void prox_open(entity &ent)
{
	if (ent.frame == 9)	// end of opening animation
	{
		// set the owner to NULL so the owner can shoot it, etc.  needs to be done here so the owner
		// doesn't get stuck on it while it's opening if fired at point blank wall
		ent.sound = SOUND_NONE;
		ent.owner = 0;

		if (ent.teamchain.has_value())
			ent.teamchain->touch = SAVABLE(Prox_Field_Touch);

		for (entity &search : G_IterateRadius(ent.origin, PROX_DAMAGE_RADIUS + 10))
		{
			if (!search.type)
				continue;

			// if it's a monster or player with health > 0
			// or it's a player start point
			// and we can see it
			// blow up
			if (((((search.svflags & SVF_MONSTER) || search.is_client()) && (search.health > 0)) ||
#ifdef SINGLE_PLAYER
					(deathmatch &&
#endif
						(search.type == ET_INFO_PLAYER_DEATHMATCH ||
						search.type == ET_INFO_PLAYER_START ||
#ifdef SINGLE_PLAYER
						search.type == ET_INFO_PLAYER_COOP ||
#endif
						search.type == ET_MISC_TELEPORTER_DEST))
#ifdef SINGLE_PLAYER
					)
#endif
				&& visible(search, ent))
			{
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/proxwarn.wav"));
				Prox_Explode (ent);
				return;
			}
		}

		int32_t multiplier = ent.dmg / PROX_DAMAGE;
		ent.wait = level.framenum + ((PROX_TIME_TO_LIVE / multiplier) * BASE_FRAMERATE);

		ent.think = SAVABLE(prox_seek);
		ent.nextthink = level.framenum + 2200ms;
	}
	else
	{
		if (ent.frame == 0)
			gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/proxopen.wav"));

		ent.frame++;
		ent.think = SAVABLE(prox_open);
		ent.nextthink = level.framenum + 100ms;
	}
}

static void prox_land(entity &ent, entity &other, vector normal, const surface &surf) 
{
	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(ent);
		return;
	}

	if (normal)
	{
		vector land_point = ent.origin + (normal * -10.f);

		if (gi.pointcontents(land_point) & (CONTENTS_SLIME|CONTENTS_LAVA))
		{
			Prox_Explode(ent);
			return;
		}
	}

	move_type cmovetype = MOVETYPE_NONE;

	if ((other.svflags & SVF_MONSTER) || other.is_client() || (other.flags & FL_DAMAGEABLE))
	{
		if(other != ent.teammaster)
			Prox_Explode(ent);

		return;
	}
	else if (!other.is_world())
	{
		//Here we need to check to see if we can stop on this entity.
		if (!normal) // this happens if you hit a point object, maybe other cases
		{
			Prox_Explode(ent);
			return;
		}

		vector out = ClipVelocity(ent.velocity, normal, 1.5);

		if (out[2] > 60)
			return;

		cmovetype = MOVETYPE_BOUNCE;

		bool stick_ok = (other.movetype == MOVETYPE_PUSH && normal[2] > 0.7);

		// if we're here, we're going to stop on an entity
		if (stick_ok) // it's a happy entity
			ent.velocity = ent.avelocity = vec3_origin;
		// no-stick.  teflon time
		else if (normal[2] > 0.7)
			Prox_Explode(ent);

		return;
	}
	else if (other.modelindex != MODEL_WORLD)
		return;

	vector dir = vectoangles (normal);

	if (gi.pointcontents (ent.origin) & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		Prox_Explode (ent);
		return;
	}

	entity &field = G_Spawn();
	field.origin = ent.origin;
	field.bounds = PROX_BOUND_SIZE;
	field.movetype = MOVETYPE_NONE;
	field.solid = SOLID_TRIGGER;
	field.owner = ent;
	field.teammaster = ent;
	gi.linkentity (field);

	ent.velocity = ent.avelocity = vec3_origin;
	// rotate to vertical
	dir[PITCH] += 90;
	ent.angles = dir;
	ent.takedamage = true;
	ent.movetype = cmovetype;		// either bounce or none, depending on whether we stuck to something
	ent.die = SAVABLE(prox_die);
	ent.teamchain = field;
	ent.health = PROX_HEALTH;
	ent.nextthink = level.framenum + 1_hz;
	ent.think = SAVABLE(prox_open);
	ent.touch = nullptr;
	ent.solid = SOLID_BBOX;

	gi.linkentity(ent);
}

REGISTER_STATIC_SAVABLE(prox_land);

void fire_prox(entity &self, vector start, vector aimdir, int32_t dmg_multiplier, int32_t speed)
{
	vector dir = vectoangles(aimdir);
	vector right, up;
	AngleVectors (dir, nullptr, &right, &up);

	entity &prox = G_Spawn();
	prox.origin = start;
	prox.velocity = aimdir * speed;
	prox.velocity += random(190.f, 210.f) * up;
	prox.velocity += random(-10.f, 10.f) * right;
	prox.angles = dir;
	prox.angles[PITCH] -= 90;
	prox.movetype = MOVETYPE_BOUNCE;
	prox.solid = SOLID_BBOX; 
	prox.effects |= EF_GRENADE;
	prox.clipmask = MASK_SHOT|CONTENTS_LAVA|CONTENTS_SLIME;
	prox.renderfx |= RF_IR_VISIBLE;
	prox.bounds = bbox::sized(6.f);
	prox.modelindex = gi.modelindex ("models/weapons/g_prox/tris.md2");
	prox.owner = self;
	prox.teammaster = self;
	prox.touch = SAVABLE(prox_land);
	prox.think = SAVABLE(Prox_Explode);
	prox.dmg = PROX_DAMAGE * dmg_multiplier;
	prox.type = ET_PROX;
	prox.bleed_style = BLEED_MECHANICAL;
	prox.flags |= FL_DAMAGEABLE;
	prox.nextthink = level.framenum + (gtime)((PROX_TIME_TO_LIVE / dmg_multiplier) * BASE_FRAMERATE);

	gi.linkentity (prox);
}
#endif