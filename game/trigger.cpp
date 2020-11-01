#include "../lib/types.h"
#include "../lib/entity.h"
#include "../lib/gi.h"
#include "trigger.h"
#include "util.h"
#include "combat.h"
#include "spawn.h"

constexpr spawn_flag TRIGGER_MONSTER = (spawn_flag)1;
constexpr spawn_flag TRIGGER_NOT_PLAYER = (spawn_flag)2;
constexpr spawn_flag TRIGGER_TRIGGERED = (spawn_flag)4;
#ifdef GROUND_ZERO
constexpr spawn_flag TRIGGER_TOGGLE = (spawn_flag)8;
#endif

void InitTrigger(entity &self)
{
	if (self.s.angles)
		G_SetMovedir(self.s.angles, self.g.movedir);

	self.solid = SOLID_TRIGGER;
	self.g.movetype = MOVETYPE_NONE;
	gi.setmodel(self, self.g.model);
	self.svflags = SVF_NOCLIENT;
}

// the wait time has passed, so set back up for another activation
static void multi_wait(entity &ent)
{
	ent.g.nextthink = 0;
}

// the trigger was just activated
// ent.activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
static void multi_trigger(entity &ent)
{
	if (ent.g.nextthink)
		return;     // already been triggered

	G_UseTargets(ent, ent.g.activator);

	if (ent.g.wait > 0)
	{
		ent.g.think = multi_wait;
		ent.g.nextthink = level.framenum + (gtime)(ent.g.wait * BASE_FRAMERATE);
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent.g.touch = 0;
		ent.g.nextthink = level.framenum + 1;
		ent.g.think = G_FreeEdict;
	}
}

static void Use_Multi(entity &ent, entity &, entity &cactivator)
{
#ifdef GROUND_ZERO
	if (ent.spawnflags & TRIGGER_TOGGLE)
	{
		if (ent.solid == SOLID_TRIGGER)
			ent.solid = SOLID_NOT;
		else
			ent.solid = SOLID_TRIGGER;
		gi.linkentity (ent);
	}
	else
	{
#endif
		ent.g.activator = cactivator;
		multi_trigger(ent);
#ifdef GROUND_ZERO
	}
#endif
}

static void Touch_Multi(entity &self, entity &other, vector, const surface &)
{
	if (other.is_client())
	{
		if (self.g.spawnflags & TRIGGER_NOT_PLAYER)
			return;
	}
	else if (other.svflags & SVF_MONSTER)
	{
		if (!(self.g.spawnflags & TRIGGER_MONSTER))
			return;
	}
	else
		return;

	if (self.g.movedir)
	{
		vector	forward;
		AngleVectors(other.s.angles, &forward, nullptr, nullptr);

		if (forward * self.g.movedir < 0)
			return;
	}

	self.g.activator = other;
	multi_trigger(self);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
sounds
1)  secret
2)  beep beep
3)  large switch
4)
set "message" to text string
*/
static void trigger_enable(entity &self, entity &, entity &)
{
	self.solid = SOLID_TRIGGER;
	self.g.use = Use_Multi;
	gi.linkentity(self);
}

static void SP_trigger_multiple(entity &ent)
{
	if (ent.g.sounds == 1)
		ent.g.noise_index = gi.soundindex("misc/secret.wav");
	else if (ent.g.sounds == 2)
		ent.g.noise_index = gi.soundindex("misc/talk.wav");
	else if (ent.g.sounds == 3)
		ent.g.noise_index = gi.soundindex("misc/trigger1.wav");

	if (!ent.g.wait)
		ent.g.wait = 0.2f;
	ent.g.touch = Touch_Multi;
	ent.g.movetype = MOVETYPE_NONE;
	ent.svflags |= SVF_NOCLIENT;


#ifdef GROUND_ZERO
	if (ent.g.spawnflags & (TRIGGER_TRIGGERED | TRIGGER_TOGGLE))
#else
	if (ent.g.spawnflags & TRIGGER_TRIGGERED)
#endif
	{
		ent.solid = SOLID_NOT;
		ent.g.use = trigger_enable;
	}
	else
	{
		ent.solid = SOLID_TRIGGER;
		ent.g.use = Use_Multi;
	}

	if (ent.s.angles)
		G_SetMovedir(ent.s.angles, ent.g.movedir);

	gi.setmodel(ent, ent.g.model);
	gi.linkentity(ent);
}

REGISTER_ENTITY(trigger_multiple, ET_TRIGGER_MULTIPLE);

/*QUAKED trigger_once (.5 .5 .5) ? x x TRIGGERED
Triggers once, then removes itself.
You must set the key "target" to the name of another object in the level that has a matching "targetname".

If TRIGGERED, this trigger must be triggered before it is live.

sounds
 1) secret
 2) beep beep
 3) large switch
 4)

"message"   string to be displayed when triggered
*/
static void SP_trigger_once(entity &ent)
{
	// make old maps work because I messed up on flag assignments here
	// triggered was on bit 1 when it should have been on bit 4
	if (ent.g.spawnflags & TRIGGER_MONSTER)
	{
		vector	v;

		v = ent.mins + (0.5f * ent.size);
		ent.g.spawnflags &= ~TRIGGER_MONSTER;
		ent.g.spawnflags |= TRIGGER_TRIGGERED;
		gi.dprintf("fixed TRIGGERED flag on %s at %s\n", st.classname.ptr(), vtos(v).ptr());
	}

	ent.g.wait = -1.f;
	SP_trigger_multiple(ent);
}

REGISTER_ENTITY(trigger_once, ET_TRIGGER_ONCE);

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.
*/
static void trigger_relay_use(entity &self, entity &, entity &cactivator)
{
	G_UseTargets(self, cactivator);
}

static void SP_trigger_relay(entity &self)
{
	self.g.use = trigger_relay_use;
}

REGISTER_ENTITY(trigger_relay, ET_TRIGGER_RELAY);

#ifdef SINGLE_PLAYER
/*
==============================================================================

trigger_key

==============================================================================
*/

/*QUAKED trigger_key (.5 .5 .5) (-8 -8 -8) (8 8 8)
A relay trigger that only fires it's targets if player has the proper key.
Use "item" to specify the required key, for example "key_data_cd"
*/
static void(entity self, entity other, entity cactivator) trigger_key_use =
{
	if (!self.item)
		return;
	if (!cactivator.is_client)
		return;

	int index = self.item->id;
	if (!cactivator.client.pers.inventory[index])
	{
		if (level.framenum < self.touch_debounce_framenum)
			return;
		self.touch_debounce_framenum = level.framenum + (int)(5.0f * BASE_FRAMERATE);
		gi.centerprintf(cactivator, "You need the %s", self.item->pickup_name);
		gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/keytry.wav"), 1, ATTN_NORM, 0);
		return;
	}

	gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/keyuse.wav"), 1, ATTN_NORM, 0);

	if (coop.intVal)
	{
		int     player;
		entity	ent;

		if (self.item->id == ITEM_POWER_CUBE)
		{
			int cube;

			for (cube = 0; cube < 8; cube++)
				if (cactivator.client.pers.power_cubes & (1 << cube))
					break;

			for (player = 1; player <= game.maxclients; player++)
			{
				ent = itoe(player);
				if (!ent.inuse)
					continue;
				if (!ent.is_client)
					continue;
				if (ent.client.pers.power_cubes & (1 << cube))
				{
					ent.client.pers.inventory[index]--;
					ent.client.pers.power_cubes &= ~(1 << cube);
				}
			}
		}
		else
		{
			for (player = 1; player <= game.maxclients; player++)
			{
				ent = itoe(player);
				if (!ent.inuse)
					continue;
				if (!ent.is_client)
					continue;
				ent.client.pers.inventory[index] = 0;
			}
		}
	}
	else
		cactivator.client.pers.inventory[index]--;

	G_UseTargets(self, cactivator);

	self.use = 0;
}

API_FUNC static void(entity self) SP_trigger_key =
{
	if (!st.item)
	{
		gi.dprintf("no key item for trigger_key at %s\n", vtos(self.s.origin));
		return;
	}
	
	self.item = FindItemByClassname(st.item);

	if (!self.item)
	{
		gi.dprintf("item %s not found for trigger_key at %s\n", st.item, vtos(self.s.origin));
		return;
	}

	if (!self.target)
	{
		gi.dprintf("%s at %s has no target\n", self.classname, vtos(self.s.origin));
		return;
	}

	gi.soundindex("misc/keytry.wav");
	gi.soundindex("misc/keyuse.wav");

	self.use = trigger_key_use;
}
#endif

/*
==============================================================================

trigger_counter

==============================================================================
*/

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.

If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.
*/
constexpr spawn_flag COUNTER_NOMESSAGE = (spawn_flag)1;

static void trigger_counter_use(entity &self, entity &, entity &cactivator)
{
	if (self.g.count == 0)
		return;

	self.g.count--;

	if (self.g.count)
	{
		if (!(self.g.spawnflags & COUNTER_NOMESSAGE))
		{
			gi.centerprintf(cactivator, "%i more to go...", self.g.count);
			gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
		}

		return;
	}

	if (!(self.g.spawnflags & COUNTER_NOMESSAGE))
	{
		gi.centerprintf(cactivator, "Sequence completed!");
		gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

	self.g.activator = cactivator;
	multi_trigger(self);
}

static void SP_trigger_counter(entity &self)
{
	self.g.wait = -1.f;
	if (!self.g.count)
		self.g.count = 2;

	self.g.use = trigger_counter_use;
}

REGISTER_ENTITY(trigger_counter, ET_TRIGGER_COUNTER);

/*
==============================================================================

trigger_always

==============================================================================
*/

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
static void SP_trigger_always(entity &ent)
{
	// we must have some delay to make sure our use targets are present
	if (ent.g.delay < 0.2f)
		ent.g.delay = 0.2f;
	G_UseTargets(ent, ent);
}

REGISTER_ENTITY(trigger_always, ET_TRIGGER_ALWAYS);

/*
==============================================================================

trigger_push

==============================================================================
*/

constexpr spawn_flag PUSH_ONCE = (spawn_flag)1;
constexpr spawn_flag PUSH_START_OFF = (spawn_flag)2;
constexpr spawn_flag PUSH_SILENT = (spawn_flag)4;

static sound_index windsound;

static void trigger_push_touch(entity &self, entity &other, vector, const surface &)
{
	if (other.g.type == ET_GRENADE || other.g.type == ET_HANDGRENADE)
		other.g.velocity = self.g.movedir * (self.g.speed * 10);
	else if (other.g.health > 0)
	{
		other.g.velocity = self.g.movedir * (self.g.speed * 10);

		if (other.is_client())
		{
			// don't take falling damage immediately from this
			other.client->g.oldvelocity = other.g.velocity;
			if (
#ifdef GROUND_ZERO
				!(self.spawnflags & PUSH_SILENT) && 
#endif
				other.g.fly_sound_debounce_framenum < level.framenum)
			{
				other.g.fly_sound_debounce_framenum = level.framenum + (gtime)(1.5f * BASE_FRAMERATE);
				gi.sound(other, CHAN_AUTO, windsound, 1, ATTN_NORM, 0);
			}
		}
	}

	if (self.g.spawnflags & PUSH_ONCE)
		G_FreeEdict(self);
}

#ifdef GROUND_ZERO
static void(entity self, entity other, entity cactivator) trigger_push_use =
{
	if (self.solid == SOLID_NOT)
		self.solid = SOLID_TRIGGER;
	else
		self.solid = SOLID_NOT;
	gi.linkentity (self);
}
#endif

#ifdef THE_RECKONING
static void(entity self) trigger_effect =
{
	vector	origin;
	int	i;

	origin = self.absmin + (self.size * 0.5);
	
	for (i=0; i<10; i++)
	{
		origin[2] += (self.speed * 0.01f) * (i + random());
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_TUNNEL_SPARKS);
		gi.WriteByte (1);
		gi.WritePosition (origin);
		gi.WriteDir (vec3_origin);
		gi.WriteByte (0x74 + (Q_rand()&7));
		gi.multicast (self.s.origin, MULTICAST_PVS);
	}
}

static void(entity self) trigger_push_active;

static void(entity self) trigger_push_inactive =
{
	if (self.delay > level.time)
	{
		self.nextthink = level.framenum + 1;
	}
	else
	{
		self.touch = trigger_push_touch;
		self.think = trigger_push_active;
		self.nextthink = level.framenum + 1;
		self.delay = self.nextthink + self.wait;  
	}
}

static void(entity self) trigger_push_active =
{
	if (self.delay > level.time)
	{
		self.nextthink = level.framenum + 1;
		trigger_effect (self);
	}
	else
	{
		self.touch = 0;
		self.think = trigger_push_inactive;
		self.nextthink = level.framenum + 1;
		self.delay = self.nextthink + self.wait;  
	}
}
#endif

/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE
Pushes the player
"speed"     defaults to 1000
*/
static void SP_trigger_push(entity &self)
{
	InitTrigger(self);
	windsound = gi.soundindex("misc/windfly.wav");
	self.g.touch = trigger_push_touch;
	
#ifdef GROUND_ZERO
	if (self.targetname)		// toggleable
	{
		self.use = trigger_push_use;
		if (self.spawnflags & PUSH_START_OFF)
			self.solid = SOLID_NOT;
	}
#endif
#ifdef THE_RECKONING
#ifdef GROUND_ZERO
	else
#endif
	if (self.spawnflags & 2)
	{
		if (!self.wait)
			self.wait = 10f;
  
		self.think = trigger_push_active;
		self.nextthink = level.framenum + 1;
		self.delay = self.nextthink + self.wait;
	}
#endif
	
	if (!self.g.speed)
		self.g.speed = 1000.f;
	gi.linkentity(self);
}

REGISTER_ENTITY(trigger_push, ET_TRIGGER_PUSH);

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.

It does dmg points of damage each server frame

SILENT          supresses playing the sound
SLOW            changes the damage rate to once per second
NO_PROTECTION   *nothing* stops the damage

"dmg"           default 5 (whole numbers only)

*/

constexpr spawn_flag HURT_START_OFF = (spawn_flag)1;
constexpr spawn_flag HURT_TOGGLE = (spawn_flag)2;
constexpr spawn_flag HURT_SILENT = (spawn_flag)4;
constexpr spawn_flag HURT_NO_PROTECTION = (spawn_flag)8;
constexpr spawn_flag HURT_SLOW = (spawn_flag)16;

static void hurt_use(entity &self, entity &, entity &)
{
	if (self.solid == SOLID_NOT)
		self.solid = SOLID_TRIGGER;
	else
		self.solid = SOLID_NOT;

	gi.linkentity(self);

	if (!(self.g.spawnflags & HURT_TOGGLE))
		self.g.use = 0;
}

static void hurt_touch(entity &self, entity &other, vector, const surface &)
{
	if (!other.g.takedamage)
		return;

	if (self.g.timestamp > level.framenum)
		return;

	if (self.g.spawnflags & HURT_SLOW)
		self.g.timestamp = level.framenum + 1 * BASE_FRAMERATE;
	else
		self.g.timestamp = level.framenum + 1;

	if (!(self.g.spawnflags & HURT_SILENT))
		if ((level.framenum % 10) == 0)
			gi.sound(other, CHAN_AUTO, self.g.noise_index, 1, ATTN_NORM, 0);

	damage_flags dflags;

	if (self.g.spawnflags & HURT_NO_PROTECTION)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = DAMAGE_NONE;

	T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, self.g.dmg, self.g.dmg, dflags, MOD_TRIGGER_HURT);
}

static void SP_trigger_hurt(entity &self)
{
	InitTrigger(self);

	self.g.noise_index = gi.soundindex("world/electro.wav");
	self.g.touch = hurt_touch;

	if (!self.g.dmg)
		self.g.dmg = 5;

	if (self.g.spawnflags & HURT_START_OFF)
		self.solid = SOLID_NOT;
	else
		self.solid = SOLID_TRIGGER;

	if (self.g.spawnflags & HURT_TOGGLE)
		self.g.use = hurt_use;

	gi.linkentity(self);
}

REGISTER_ENTITY(trigger_hurt, ET_TRIGGER_HURT);

/*
==============================================================================

trigger_gravity

==============================================================================
*/

/*QUAKED trigger_gravity (.5 .5 .5) ?
Changes the touching entites gravity to
the value of "gravity".  1.0 is standard
gravity for the level.
*/

#ifdef GROUND_ZERO
static void(entity self, entity other, entity cactivator) trigger_gravity_use =
{
	if (self.solid == SOLID_NOT)
		self.solid = SOLID_TRIGGER;
	else
		self.solid = SOLID_NOT;
	gi.linkentity (self);
}
#endif

static void trigger_gravity_touch(entity &self, entity &other, vector, const surface &)
{
	other.g.gravity = self.g.gravity;
}

static void SP_trigger_gravity(entity &self)
{
	if (!st.gravity)
	{
		gi.dprintf("trigger_gravity without gravity set at %s\n", vtos(self.s.origin).ptr());
		G_FreeEdict(self);
		return;
	}

	InitTrigger(self);
	self.g.gravity = (float)atof(st.gravity);
	
#ifdef GROUND_ZERO
	if(self.spawnflags & 1)				// TOGGLE
		self.use = trigger_gravity_use;

	if(self.spawnflags & 2)				// START_OFF
	{
		self.use = trigger_gravity_use;
		self.solid = SOLID_NOT;
	}
#endif
	
	self.g.touch = trigger_gravity_touch;
	gi.linkentity(self);
}

REGISTER_ENTITY(trigger_gravity, ET_TRIGGER_GRAVITY);

#ifdef SINGLE_PLAYER
/*
==============================================================================

trigger_monsterjump

==============================================================================
*/

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards
*/
static void(entity self, entity other, vector plane, csurface_t surf) trigger_monsterjump_touch =
{
	if (other.flags & (FL_FLY | FL_SWIM))
		return;
	if (other.svflags & SVF_DEADMONSTER)
		return;
	if (!(other.svflags & SVF_MONSTER))
		return;

// set XY even if not on ground, so the jump will clear lips
	other.velocity.x = self.movedir.x * self.speed;
	other.velocity.y = self.movedir.y * self.speed;

	if (other.groundentity == null_entity)
		return;

	other.groundentity = null_entity;
	other.velocity.z = self.movedir.z;
}

API_FUNC static void(entity self) SP_trigger_monsterjump =
{
	if (!self.speed)
		self.speed = 200f;
	if (!st.height)
		st.height = 200;
	if (self.s.angles[YAW] == 0)
		self.s.angles[YAW] = 360f;
	InitTrigger(self);
	self.touch = trigger_monsterjump_touch;
	self.movedir.z = (float)st.height;
}
#endif
