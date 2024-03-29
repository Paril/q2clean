#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/util.h"
#include "game/combat.h"
#ifdef SINGLE_PLAYER
#include "game/weaponry.h"
#include "game/ballistics.h"
#endif
#include "tracker.h"

constexpr damage_flags TRACKER_IMPACT_FLAGS = (DAMAGE_NO_POWER_ARMOR | DAMAGE_ENERGY | DAMAGE_INSTANT_GIB);
constexpr damage_flags TRACKER_DAMAGE_FLAGS = (TRACKER_IMPACT_FLAGS | DAMAGE_NO_KNOCKBACK);

constexpr gtime TRACKER_DAMAGE_TIME	= 500ms;

constexpr means_of_death MOD_TRACKER { .other_kill_fmt = "{0} was annihilated by {3}'s disruptor.\n" };

static void tracker_pain_daemon_think(entity &self)
{
	if(!self.inuse)
		return;

	if ((level.time - self.timestamp) > TRACKER_DAMAGE_TIME)
	{
		if(!self.enemy->is_client)
			self.enemy->effects &= ~EF_TRACKERTRAIL;
		G_FreeEdict (self);
		return;
	}

	if (self.enemy->health > 0)
	{
		T_Damage (self.enemy, self, self.owner, vec3_origin, self.enemy->origin, MOVEDIR_UP, self.dmg, 0, { TRACKER_DAMAGE_FLAGS }, MOD_TRACKER);
		
		// if we kill the player, we'll be removed.
		if (!self.inuse)
			return;

		if (self.enemy->is_client)
			self.enemy->client.tracker_pain_time = level.time + 100ms;
		else
			self.enemy->effects |= EF_TRACKERTRAIL;
		
		self.nextthink = level.time + 100ms;
		return;
	}

	if (!self.enemy->is_client)
		self.enemy->effects &= ~EF_TRACKERTRAIL;

	G_FreeEdict (self);
}

REGISTER_STATIC_SAVABLE(tracker_pain_daemon_think);

static inline void tracker_pain_daemon_spawn(entity &cowner, entity &cenemy, int32_t damage)
{
	entity &daemon = G_Spawn();
	daemon.think = SAVABLE(tracker_pain_daemon_think);
	daemon.nextthink = level.time + 100ms;
	daemon.timestamp = level.time;
	daemon.owner = cowner;
	daemon.enemy = cenemy;
	daemon.dmg = damage;
}

static inline void tracker_explode(entity &self)
{
	gi.ConstructMessage(svc_temp_entity, TE_TRACKER_EXPLOSION, self.origin).multicast(self.origin, MULTICAST_PVS);

	G_FreeEdict (self);
}

static void tracker_touch(entity &self, entity &other, vector normal, const surface &surf)
{
	if (other == self.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict (self);
		return;
	}
#if defined(SINGLE_PLAYER)

	if (self.is_client)
		PlayerNoise(self.owner, self.origin, PNOISE_IMPACT);
#endif

	if (other.takedamage)
	{
		if ((other.svflags & SVF_MONSTER) || other.is_client)
		{
			if (other.health > 0)		// knockback only for living creatures
			{
				// PMM - kickback was times 4 .. reduced to 3
				// now this does no damage, just knockback
				T_Damage (other, self, self.owner, self.velocity, self.origin, normal,
							0, (self.dmg*3), { TRACKER_IMPACT_FLAGS }, MOD_TRACKER);

#ifdef SINGLE_PLAYER
				if (!(other.flags & (FL_FLY|FL_SWIM)))
#endif
					other.velocity[2] += 140;
				
				float damagetime = (self.dmg * FRAMETIME) / TRACKER_DAMAGE_TIME;
				tracker_pain_daemon_spawn (self.owner, other, (int)damagetime);
			}
			else						// lots of damage (almost autogib) for dead bodies
			{
				T_Damage (other, self, self.owner, self.velocity, self.origin, normal,
							self.dmg*4, (self.dmg*3), { TRACKER_IMPACT_FLAGS }, MOD_TRACKER);
			}
		}
		else	// full damage in one shot for inanimate objects
		{
			T_Damage (other, self, self.owner, self.velocity, self.origin, normal,
						self.dmg, (self.dmg*3), { TRACKER_IMPACT_FLAGS }, MOD_TRACKER);
		}
	}

	tracker_explode (self);
}

REGISTER_STATIC_SAVABLE(tracker_touch);

static void tracker_fly(entity &self)
{
	if (!self.enemy.has_value() || !self.enemy->inuse || (self.enemy->health < 1))
	{
		tracker_explode (self);
		return;
	}

	// PMM - try to hunt for center of enemy, if possible and not client
	vector dest;

	if (self.enemy->is_client)
	{
		dest = self.enemy->origin;
		dest[2] += self.enemy->viewheight;
	}
	else
		dest = self.enemy->absbounds.center();

	vector dir = dest - self.origin;
	VectorNormalize (dir);
	self.angles = vectoangles(dir);
	self.velocity = dir * self.speed;

	self.nextthink = level.time + 1_hz;
}

REGISTER_STATIC_SAVABLE(tracker_fly);

void fire_tracker(entity &self, vector start, vector dir, int32_t damage, int32_t cspeed, entityref cenemy)
{
	VectorNormalize (dir);

	entity &bolt = G_Spawn();
	bolt.origin = bolt.old_origin = start;
	bolt.angles = vectoangles (dir);
	bolt.velocity = dir * cspeed;
	bolt.movetype = MOVETYPE_FLYMISSILE;
	bolt.clipmask = MASK_SHOT;
	bolt.solid = SOLID_BBOX;
	bolt.speed = cspeed;
	bolt.effects = EF_TRACKER;
	bolt.sound = gi.soundindex ("weapons/disrupt.wav");
	
	bolt.modelindex = gi.modelindex ("models/proj/disintegrator/tris.md2");
	bolt.touch = SAVABLE(tracker_touch);
	bolt.enemy = cenemy;
	bolt.owner = self;
	bolt.dmg = damage;
	gi.linkentity (bolt);

	if (cenemy.has_value())
	{
		bolt.nextthink = level.time + 1_hz;
		bolt.think = SAVABLE(tracker_fly);
	}
	else
	{
		bolt.nextthink = level.time + 10s;
		bolt.think = SAVABLE(G_FreeEdict);
	}

#ifdef SINGLE_PLAYER
	if (self.is_client)
		check_dodge (self, bolt.origin, dir, cspeed);
#endif

	trace tr = gi.traceline(self.origin, bolt.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		bolt.origin += dir * -10;
		bolt.touch (bolt, tr.ent, vec3_origin, null_surface);
	}
}
#endif