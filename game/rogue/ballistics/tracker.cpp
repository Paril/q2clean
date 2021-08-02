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

constexpr damage_flags TRACKER_DAMAGE_FLAGS = (DAMAGE_NO_POWER_ARMOR | DAMAGE_ENERGY | DAMAGE_NO_KNOCKBACK);
constexpr damage_flags TRACKER_IMPACT_FLAGS = (DAMAGE_NO_POWER_ARMOR | DAMAGE_ENERGY);

constexpr float TRACKER_DAMAGE_TIME	= 0.5f; // time in seconds

static void tracker_pain_daemon_think(entity &self)
{
	if(!self.inuse)
		return;

	if ((level.framenum - self.timestamp) > TRACKER_DAMAGE_TIME * BASE_FRAMERATE)
	{
		if(!self.enemy->is_client())
			self.enemy->s.effects &= ~EF_TRACKERTRAIL;
		G_FreeEdict (self);
		return;
	}

	if (self.enemy->health > 0)
	{
		T_Damage (self.enemy, self, self.owner, vec3_origin, self.enemy->s.origin, MOVEDIR_UP, self.dmg, 0, TRACKER_DAMAGE_FLAGS, MOD_TRACKER);
		
		// if we kill the player, we'll be removed.
		if (!self.inuse)
			return;

		// if we killed a monster, gib them.
		if (self.enemy->health < 1)
		{
			int hurt;
			
			if(self.enemy->gib_health)
				hurt = -self.enemy->gib_health;
			else
				hurt = 500;

			T_Damage (self.enemy, self, self.owner, vec3_origin, self.enemy->s.origin, MOVEDIR_UP, hurt, 0, TRACKER_DAMAGE_FLAGS, MOD_TRACKER);
		}

		if (self.enemy->is_client())
			self.enemy->client->tracker_pain_framenum = level.framenum + 1;
		else
			self.enemy->s.effects |= EF_TRACKERTRAIL;
		
		self.nextthink = level.framenum + 1;
		return;
	}

	if (!self.enemy->is_client())
		self.enemy->s.effects &= ~EF_TRACKERTRAIL;

	G_FreeEdict (self);
}

static REGISTER_SAVABLE_FUNCTION(tracker_pain_daemon_think);

static inline void tracker_pain_daemon_spawn(entity &cowner, entity &cenemy, int32_t damage)
{
	entity &daemon = G_Spawn();
	daemon.think = SAVABLE(tracker_pain_daemon_think);
	daemon.nextthink = level.framenum + 1;
	daemon.timestamp = level.framenum;
	daemon.owner = cowner;
	daemon.enemy = cenemy;
	daemon.dmg = damage;
}

static inline void tracker_explode(entity &self)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TRACKER_EXPLOSION);
	gi.WritePosition (self.s.origin);
	gi.multicast (self.s.origin, MULTICAST_PVS);

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

	if (self.is_client())
		PlayerNoise(self.owner, self.s.origin, PNOISE_IMPACT);
#endif

	if (other.takedamage)
	{
		if ((other.svflags & SVF_MONSTER) || other.is_client())
		{
			if (other.health > 0)		// knockback only for living creatures
			{
				// PMM - kickback was times 4 .. reduced to 3
				// now this does no damage, just knockback
				T_Damage (other, self, self.owner, self.velocity, self.s.origin, normal,
							0, (self.dmg*3), TRACKER_IMPACT_FLAGS, MOD_TRACKER);

#ifdef SINGLE_PLAYER
				if (!(other.flags & (FL_FLY|FL_SWIM)))
#endif
					other.velocity[2] += 140;
				
				float damagetime = (self.dmg * FRAMETIME) / TRACKER_DAMAGE_TIME;
				tracker_pain_daemon_spawn (self.owner, other, (int)damagetime);
			}
			else						// lots of damage (almost autogib) for dead bodies
			{
				T_Damage (other, self, self.owner, self.velocity, self.s.origin, normal,
							self.dmg*4, (self.dmg*3), TRACKER_IMPACT_FLAGS, MOD_TRACKER);
			}
		}
		else	// full damage in one shot for inanimate objects
		{
			T_Damage (other, self, self.owner, self.velocity, self.s.origin, normal,
						self.dmg, (self.dmg*3), TRACKER_IMPACT_FLAGS, MOD_TRACKER);
		}
	}

	tracker_explode (self);
}

static REGISTER_SAVABLE_FUNCTION(tracker_touch);

static void tracker_fly(entity &self)
{
	if (!self.enemy.has_value() || !self.enemy->inuse || (self.enemy->health < 1))
	{
		tracker_explode (self);
		return;
	}

	// PMM - try to hunt for center of enemy, if possible and not client
	vector dest;

	if (self.enemy->is_client())
	{
		dest = self.enemy->s.origin;
		dest[2] += self.enemy->viewheight;
	}
	else
		dest = self.enemy->absbounds.center();

	vector dir = dest - self.s.origin;
	VectorNormalize (dir);
	self.s.angles = vectoangles(dir);
	self.velocity = dir * self.speed;

	self.nextthink = level.framenum + 1;
}

static REGISTER_SAVABLE_FUNCTION(tracker_fly);

void fire_tracker(entity &self, vector start, vector dir, int32_t damage, int32_t cspeed, entityref cenemy)
{
	VectorNormalize (dir);

	entity &bolt = G_Spawn();
	bolt.s.origin = bolt.s.old_origin = start;
	bolt.s.angles = vectoangles (dir);
	bolt.velocity = dir * cspeed;
	bolt.movetype = MOVETYPE_FLYMISSILE;
	bolt.clipmask = MASK_SHOT;
	bolt.solid = SOLID_BBOX;
	bolt.speed = cspeed;
	bolt.s.effects = EF_TRACKER;
	bolt.s.sound = gi.soundindex ("weapons/disrupt.wav");
	
	bolt.s.modelindex = gi.modelindex ("models/proj/disintegrator/tris.md2");
	bolt.touch = SAVABLE(tracker_touch);
	bolt.enemy = cenemy;
	bolt.owner = self;
	bolt.dmg = damage;
	gi.linkentity (bolt);

	if (cenemy.has_value())
	{
		bolt.nextthink = level.framenum + 1;
		bolt.think = SAVABLE(tracker_fly);
	}
	else
	{
		bolt.nextthink = level.framenum + (10 * BASE_FRAMERATE);
		bolt.think = SAVABLE(G_FreeEdict);
	}

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge (self, bolt.s.origin, dir, cspeed);
#endif

	trace tr = gi.traceline(self.s.origin, bolt.s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		bolt.s.origin += dir * -10;
		bolt.touch (bolt, tr.ent, vec3_origin, null_surface);
	}
}
#endif