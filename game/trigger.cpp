#include "config.h"
#include "entity.h"
#include "trigger.h"
#include "spawn.h"
#include "game.h"

#include "lib/gi.h"
#include "game.h"
#include "util.h"
#include "lib/string/format.h"
#include "combat.h"
#include "game/ballistics/grenade.h"

constexpr spawn_flag TRIGGER_MONSTER = (spawn_flag)1;
constexpr spawn_flag TRIGGER_NOT_PLAYER = (spawn_flag)2;
constexpr spawn_flag TRIGGER_TRIGGERED = (spawn_flag)4;
#ifdef GROUND_ZERO
constexpr spawn_flag TRIGGER_TOGGLE = (spawn_flag)8;
#endif

void InitTrigger(entity &self)
{
	if (self.angles)
		G_SetMovedir(self.angles, self.movedir);

	self.solid = SOLID_TRIGGER;
	self.movetype = MOVETYPE_NONE;
	gi.setmodel(self, self.model);
	self.svflags = SVF_NOCLIENT;
}

// the wait time has passed, so set back up for another activation
static void multi_wait(entity &ent)
{
	ent.nextthink = gtime::zero();
}

REGISTER_STATIC_SAVABLE(multi_wait);

// the trigger was just activated
// ent.activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
static void multi_trigger(entity &ent)
{
	if (ent.nextthink != gtime::zero())
		return;     // already been triggered

	G_UseTargets(ent, ent.activator);

	if (ent.wait.count() > 0)
	{
		ent.think = SAVABLE(multi_wait);
		ent.nextthink = duration_cast<gtime>(level.framenum + ent.wait);
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent.touch = nullptr;
		ent.nextthink = level.framenum + 1_hz;
		ent.think = SAVABLE(G_FreeEdict);
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
		ent.activator = cactivator;
		multi_trigger(ent);
#ifdef GROUND_ZERO
	}
#endif
}

REGISTER_STATIC_SAVABLE(Use_Multi);

static void Touch_Multi(entity &self, entity &other, vector, const surface &)
{
	if (other.is_client())
	{
		if (self.spawnflags & TRIGGER_NOT_PLAYER)
			return;
	}
	else if (other.svflags & SVF_MONSTER)
	{
		if (!(self.spawnflags & TRIGGER_MONSTER))
			return;
	}
	else
		return;

	if (self.movedir)
	{
		vector	forward;
		AngleVectors(other.angles, &forward, nullptr, nullptr);

		if (forward * self.movedir < 0)
			return;
	}

	self.activator = other;
	multi_trigger(self);
}

REGISTER_STATIC_SAVABLE(Touch_Multi);

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
	self.use = SAVABLE(Use_Multi);
	gi.linkentity(self);
}

REGISTER_STATIC_SAVABLE(trigger_enable);

static void SP_trigger_multiple(entity &ent)
{
	if (ent.sounds == 1)
		ent.noise_index = gi.soundindex("misc/secret.wav");
	else if (ent.sounds == 2)
		ent.noise_index = gi.soundindex("misc/talk.wav");
	else if (ent.sounds == 3)
		ent.noise_index = gi.soundindex("misc/trigger1.wav");

	if (ent.wait == gtimef::zero())
		ent.wait = 0.2s;
	ent.touch = SAVABLE(Touch_Multi);
	ent.movetype = MOVETYPE_NONE;
	ent.svflags |= SVF_NOCLIENT;


#ifdef GROUND_ZERO
	if (ent.spawnflags & (TRIGGER_TRIGGERED | TRIGGER_TOGGLE))
#else
	if (ent.spawnflags & TRIGGER_TRIGGERED)
#endif
	{
		ent.solid = SOLID_NOT;
		ent.use = SAVABLE(trigger_enable);
	}
	else
	{
		ent.solid = SOLID_TRIGGER;
		ent.use = SAVABLE(Use_Multi);
	}

	if (ent.angles)
		G_SetMovedir(ent.angles, ent.movedir);

	gi.setmodel(ent, ent.model);
	gi.linkentity(ent);
}

static REGISTER_ENTITY(TRIGGER_MULTIPLE, trigger_multiple);

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
	if (ent.spawnflags & TRIGGER_MONSTER)
	{
		vector v = ent.bounds.center();
		ent.spawnflags &= ~TRIGGER_MONSTER;
		ent.spawnflags |= TRIGGER_TRIGGERED;
		gi.dprintfmt("{}: fixed TRIGGERED flag\n", ent);
	}

	ent.wait = -1s;
	SP_trigger_multiple(ent);
}

REGISTER_ENTITY(TRIGGER_ONCE, trigger_once);

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.
*/
static void trigger_relay_use(entity &self, entity &, entity &cactivator)
{
	G_UseTargets(self, cactivator);
}

REGISTER_STATIC_SAVABLE(trigger_relay_use);

static void SP_trigger_relay(entity &self)
{
	self.use = SAVABLE(trigger_relay_use);
}

static REGISTER_ENTITY(TRIGGER_RELAY, trigger_relay);

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
static void trigger_key_use(entity &self, entity &, entity &cactivator)
{
	if (!self.item)
		return;
	if (!cactivator.is_client())
		return;

	int index = self.item->id;
	if (!cactivator.client->pers.inventory[index])
	{
		if (level.framenum < self.touch_debounce_framenum)
			return;
		self.touch_debounce_framenum = level.framenum + 5s;
		gi.centerprintfmt(cactivator, "You need the {}", self.item->pickup_name);
		gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/keytry.wav"));
		return;
	}

	gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/keyuse.wav"));

	if (coop)
	{
		if (self.item->id == ITEM_POWER_CUBE)
		{
			int cube;

			for (cube = 0; cube < 8; cube++)
				if (cactivator.client->pers.power_cubes & (1 << cube))
					break;

			for (entity &ent : entity_range(1, game.maxclients))
			{
				if (!ent.inuse)
					continue;
				if (ent.client->pers.power_cubes & (1 << cube))
				{
					ent.client->pers.inventory[index]--;
					ent.client->pers.power_cubes &= ~(1 << cube);
				}
			}
		}
		else
		{
			for (entity &ent : entity_range(1, game.maxclients))
				if (ent.inuse)
					ent.client->pers.inventory[index] = 0;
		}
	}
	else
		cactivator.client->pers.inventory[index]--;

	G_UseTargets(self, cactivator);

	self.use = nullptr;
}

REGISTER_STATIC_SAVABLE(trigger_key_use);

static void SP_trigger_key(entity &self)
{
	if (!st.item)
	{
		gi.dprintfmt("{}: no key item\n", self);
		return;
	}
	
	self.item = FindItemByClassname(st.item);

	if (!self.item)
	{
		gi.dprintfmt("{}: item \"{}\" not found\n", self, st.item);
		return;
	}

	if (!self.target)
	{
		gi.dprintfmt("{}: no target\n", self);
		return;
	}

	gi.soundindex("misc/keytry.wav");
	gi.soundindex("misc/keyuse.wav");

	self.use = SAVABLE(trigger_key_use);
}

static REGISTER_ENTITY(TRIGGER_KEY, trigger_key);
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
	if (self.count == 0)
		return;

	self.count--;

	if (self.count)
	{
		if (!(self.spawnflags & COUNTER_NOMESSAGE))
		{
			gi.centerprintfmt(cactivator, "{} more to go...", self.count);
			gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"));
		}

		return;
	}

	if (!(self.spawnflags & COUNTER_NOMESSAGE))
	{
		gi.centerprint(cactivator, "Sequence completed!");
		gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"));
	}

	self.activator = cactivator;
	multi_trigger(self);
}

REGISTER_STATIC_SAVABLE(trigger_counter_use);

static void SP_trigger_counter(entity &self)
{
	self.wait = -1s;
	if (!self.count)
		self.count = 2;

	self.use = SAVABLE(trigger_counter_use);
}

static REGISTER_ENTITY(TRIGGER_COUNTER, trigger_counter);

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
	if (ent.delay < 0.2s)
		ent.delay = 0.2s;
	G_UseTargets(ent, ent);
}

static REGISTER_ENTITY(TRIGGER_ALWAYS, trigger_always);

/*
==============================================================================

trigger_push

==============================================================================
*/

constexpr spawn_flag PUSH_ONCE = (spawn_flag) 1;
#ifdef GROUND_ZERO
constexpr spawn_flag PUSH_START_OFF = (spawn_flag) 2;
constexpr spawn_flag PUSH_SILENT = (spawn_flag) 4;
#endif

static sound_index windsound;

static void trigger_push_touch(entity &self, entity &other, vector, const surface &)
{
	if (other.type == ET_GRENADE)
		other.velocity = self.movedir * (self.speed * 10);
	else if (other.health > 0)
	{
		other.velocity = self.movedir * (self.speed * 10);

		if (other.is_client())
		{
			// don't take falling damage immediately from this
			other.client->oldvelocity = other.velocity;
			if (
#ifdef GROUND_ZERO
				!(self.spawnflags & PUSH_SILENT) && 
#endif
				other.fly_sound_debounce_framenum < level.framenum)
			{
				other.fly_sound_debounce_framenum = duration_cast<gtime>(level.framenum + 1.5s);
				gi.sound(other, CHAN_AUTO, windsound);
			}
		}
	}

	if (self.spawnflags & PUSH_ONCE)
		G_FreeEdict(self);
}

REGISTER_STATIC_SAVABLE(trigger_push_touch);

#ifdef GROUND_ZERO
static void trigger_push_use(entity &self, entity &, entity &)
{
	if (self.solid == SOLID_NOT)
		self.solid = SOLID_TRIGGER;
	else
		self.solid = SOLID_NOT;
	gi.linkentity (self);
}

REGISTER_STATIC_SAVABLE(trigger_push_use);
#endif

#ifdef THE_RECKONING
#include "lib/math/random.h"

static void trigger_effect(entity &self)
{
	vector origin = self.absbounds.center();

	for (int32_t i = 0; i < 10; i++)
	{
		origin[2] += (self.speed * 0.01f) * (i + random());
		gi.ConstructMessage(svc_temp_entity, TE_TUNNEL_SPARKS, uint8_t { 1 }, origin, vecdir { vec3_origin }, uint8_t { 0x74 + (Q_rand() & 7) }).multicast(self.origin, MULTICAST_PVS);
	}
}

static void trigger_push_active(entity &self);

REGISTER_STATIC_SAVABLE(trigger_push_active);

static void trigger_push_inactive(entity &self)
{
	if (self.delay > level.time)
		self.nextthink = level.framenum + 1_hz;
	else
	{
		self.touch = SAVABLE(trigger_push_touch);
		self.think = SAVABLE(trigger_push_active);
		self.nextthink = level.framenum + 1_hz;
		self.delay = self.nextthink + self.wait;  
	}
}

REGISTER_STATIC_SAVABLE(trigger_push_inactive);

static void trigger_push_active(entity &self)
{
	if (self.delay > level.time)
	{
		self.nextthink = level.framenum + 1_hz;
		trigger_effect (self);
	}
	else
	{
		self.touch = nullptr;
		self.think = SAVABLE(trigger_push_inactive);
		self.nextthink = level.framenum + 1_hz;
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
	self.touch = SAVABLE(trigger_push_touch);
	
#ifdef GROUND_ZERO
	if (self.targetname)		// toggleable
	{
		self.use = SAVABLE(trigger_push_use);
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
		if (self.wait == gtime::zero())
			self.wait = 10s;
  
		self.think = SAVABLE(trigger_push_active);
		self.nextthink = level.framenum + 1_hz;
		self.delay = self.nextthink + self.wait;
	}
#endif
	
	if (!self.speed)
		self.speed = 1000.f;
	gi.linkentity(self);
}

static REGISTER_ENTITY(TRIGGER_PUSH, trigger_push);

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

	if (!(self.spawnflags & HURT_TOGGLE))
		self.use = nullptr;
}

REGISTER_STATIC_SAVABLE(hurt_use);

constexpr means_of_death MOD_TRIGGER_HURT { .self_kill_fmt = "{0} was in the wrong place.\n" };

static void hurt_touch(entity &self, entity &other, vector, const surface &)
{
	if (!other.takedamage)
		return;

	if (self.timestamp > level.framenum)
		return;

	if (self.spawnflags & HURT_SLOW)
		self.timestamp = level.framenum + 1s;
	else
		self.timestamp = level.framenum + 100ms;

	if (!(self.spawnflags & HURT_SILENT))
		if ((level.framenum % 1000ms) == gtime::zero())
			gi.sound(other, CHAN_AUTO, self.noise_index);

	damage_flags dflags;

	if (self.spawnflags & HURT_NO_PROTECTION)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = DAMAGE_NONE;

	T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, self.dmg, self.dmg, { dflags }, MOD_TRIGGER_HURT);
}

REGISTER_STATIC_SAVABLE(hurt_touch);

static void SP_trigger_hurt(entity &self)
{
	InitTrigger(self);

	self.noise_index = gi.soundindex("world/electro.wav");
	self.touch = SAVABLE(hurt_touch);

	if (!self.dmg)
		self.dmg = 5;

	if (self.spawnflags & HURT_START_OFF)
		self.solid = SOLID_NOT;
	else
		self.solid = SOLID_TRIGGER;

	if (self.spawnflags & HURT_TOGGLE)
		self.use = SAVABLE(hurt_use);

	gi.linkentity(self);
}

static REGISTER_ENTITY(TRIGGER_HURT, trigger_hurt);

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
static void trigger_gravity_use(entity &self, entity &, entity &)
{
	if (self.solid == SOLID_NOT)
		self.solid = SOLID_TRIGGER;
	else
		self.solid = SOLID_NOT;
	gi.linkentity (self);
}

REGISTER_STATIC_SAVABLE(trigger_gravity_use);

static constexpr spawn_flag GRAVITY_TOGGLE = (spawn_flag) 1;
static constexpr spawn_flag GRAVITY_START_OFF = (spawn_flag) 2;
#endif

static void trigger_gravity_touch(entity &self, entity &other, vector, const surface &)
{
	other.gravity = self.gravity;
}

REGISTER_STATIC_SAVABLE(trigger_gravity_touch);

static void SP_trigger_gravity(entity &self)
{
	if (!st.gravity)
	{
		gi.dprintfmt("{}: no gravity set\n", self);
		G_FreeEdict(self);
		return;
	}

	InitTrigger(self);
	self.gravity = (float)atof(st.gravity);
	
#ifdef GROUND_ZERO
	if(self.spawnflags & GRAVITY_TOGGLE)
		self.use = SAVABLE(trigger_gravity_use);

	if(self.spawnflags & GRAVITY_START_OFF)
	{
		self.use = SAVABLE(trigger_gravity_use);
		self.solid = SOLID_NOT;
	}
#endif
	
	self.touch = SAVABLE(trigger_gravity_touch);
	gi.linkentity(self);
}

static REGISTER_ENTITY(TRIGGER_GRAVITY, trigger_gravity);

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
static void trigger_monsterjump_touch(entity &self, entity &other, vector, const surface &)
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

REGISTER_STATIC_SAVABLE(trigger_monsterjump_touch);

static void SP_trigger_monsterjump(entity &self)
{
	if (!self.speed)
		self.speed = 200.f;
	if (!st.height)
		st.height = 200;
	if (self.angles[YAW] == 0)
		self.angles[YAW] = 360.f;
	InitTrigger(self);
	self.touch = SAVABLE(trigger_monsterjump_touch);
	self.movedir.z = (float)st.height;
}

static REGISTER_ENTITY(TRIGGER_MONSTERJUMP, trigger_monsterjump);
#endif
