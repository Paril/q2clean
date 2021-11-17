#include "config.h"
#include "entity.h"
#include "game.h"
#include "spawn.h"

#include "lib/gi.h"
#include "game.h"
#include "util.h"
#include "hud.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "ballistics/blaster.h"
#include "combat.h"
#include "target.h"
#include "misc.h"

/*QUAKED target_temp_entity (1 0 0) (-8 -8 -8) (8 8 8)
Fire an origin based temp entity event to the clients.
"style"     type byte
*/
static void Use_Target_Tent(entity &ent, entity &, entity &)
{
	gi.ConstructMessage(svc_temp_entity, (uint8_t) ent.style, ent.origin).multicast(ent.origin, MULTICAST_PVS);
}

REGISTER_STATIC_SAVABLE(Use_Target_Tent);

static void SP_target_temp_entity(entity &ent)
{
	ent.use = SAVABLE(Use_Target_Tent);
}

static REGISTER_ENTITY(TARGET_TEMP_ENTITY, target_temp_entity);

//==========================================================

//==========================================================

/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off reliable
"noise"     wav file to play
"attenuation"
-1 = none, send to whole level
1 = normal fighting sounds
2 = idle sound level
3 = ambient sound level
"volume"    0.0 to 1.0

Normal sounds play each time the target is used.  The reliable flag can be set for crucial voiceovers.

Looped sounds are always atten 3 / vol 1, and the use function toggles it on/off.
Multiple identical looping sounds will just increase volume without any speed cost.
*/
constexpr spawn_flag SPEAKER_LOOPED_ON = (spawn_flag)1;
constexpr spawn_flag SPEAKER_LOOPED_OFF = (spawn_flag)2;
constexpr spawn_flag SPEAKER_RELIABLE = (spawn_flag)4;

static void Use_Target_Speaker(entity &ent, entity &, entity &)
{
	sound_channel chan;

	if (ent.spawnflags & (SPEAKER_LOOPED_ON | SPEAKER_LOOPED_OFF))
	{
		// looping sound toggles
		if (ent.sound)
			ent.sound = SOUND_NONE;   // turn it off
		else
			ent.sound = ent.noise_index;    // start it
	}
	else
	{
		// normal sound
		if (ent.spawnflags & SPEAKER_RELIABLE)
			chan = CHAN_VOICE | CHAN_RELIABLE;
		else
			chan = CHAN_VOICE;

		// use a positioned_sound, because this entity won't normally be
		// sent to any clients because it is invisible
		gi.positioned_sound(ent.origin, ent, chan, ent.noise_index, ent.volume, ent.attenuation);
	}
}

REGISTER_STATIC_SAVABLE(Use_Target_Speaker);

static void SP_target_speaker(entity &ent)
{
	string buffer = st.noise;

	if (!buffer)
	{
		gi.dprintfmt("{}: no noise\n", ent);
		return;
	}

	// fix bad noises
	if (strstr(st.noise, ".wav") == (size_t) -1)
		buffer = format("{}.wav", st.noise);

	ent.noise_index = gi.soundindex(buffer);

	if (!ent.volume)
		ent.volume = 1.0f;

	if (!ent.attenuation)
		ent.attenuation = ATTN_NORM;
	else if (ent.attenuation == (sound_attn) -1)    // use -1 so 0 defaults to 1
		ent.attenuation = ATTN_NONE;

	// check for prestarted looping sound
	if (ent.spawnflags & SPEAKER_LOOPED_ON)
		ent.sound = ent.noise_index;

	ent.use = SAVABLE(Use_Target_Speaker);

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity(ent);
}

static REGISTER_ENTITY(TARGET_SPEAKER, target_speaker);

#ifdef SINGLE_PLAYER
//==========================================================

static void Use_Target_Help(entity &ent, entity &, entity &)
{
	if (ent.spawnflags & 1)
		game.helpmessage1 = ent.message;
	else
		game.helpmessage2 = ent.message;

	game.helpchanged++;
}

REGISTER_STATIC_SAVABLE(Use_Target_Help);

/*QUAKED target_help (1 0 1) (-16 -16 -24) (16 16 24) help1
When fired, the "message" key becomes the current personal computer string, and the message light will be set on all clients status bars.
*/
static void SP_target_help(entity &ent)
{
	if (deathmatch)
	{
		// auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	if (!ent.message)
	{
		gi.dprintfmt("{}: no message\n", ent);
		G_FreeEdict(ent);
		return;
	}
	ent.use = SAVABLE(Use_Target_Help);
}

static REGISTER_ENTITY(TARGET_HELP, target_help);

//==========================================================

/*QUAKED target_secret (1 0 1) (-8 -8 -8) (8 8 8)
Counts a secret found.
These are single use targets.
*/
static void use_target_secret(entity &ent, entity &, entity &cactivator)
{
	gi.sound(ent, CHAN_VOICE, ent.noise_index);

	level.found_secrets++;

	G_UseTargets(ent, cactivator);
	G_FreeEdict(ent);
}

REGISTER_STATIC_SAVABLE(use_target_secret);

static void SP_target_secret(entity &ent)
{
	if (deathmatch)
	{
		// auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	ent.use = SAVABLE(use_target_secret);
	if (!st.noise)
		st.noise = "misc/secret.wav";
	ent.noise_index = gi.soundindex(st.noise);
	ent.svflags = SVF_NOCLIENT;
	level.total_secrets++;
	// map bug hack
	if (striequals(level.mapname, "mine3") && ent.origin[0] == 280 && ent.origin[1] == -2048 && ent.origin[2] == -624)
		ent.message = "You have found a secret area.";
}

static REGISTER_ENTITY(TARGET_SECRET, target_secret);

//==========================================================

/*QUAKED target_goal (1 0 1) (-8 -8 -8) (8 8 8)
Counts a goal completed.
These are single use targets.
*/
static void use_target_goal(entity &ent, entity &, entity &cactivator)
{
	gi.sound(ent, CHAN_VOICE, ent.noise_index);

	level.found_goals++;

	if (level.found_goals == level.total_goals)
		gi.configstring(CS_CDTRACK, "0");

	G_UseTargets(ent, cactivator);
	G_FreeEdict(ent);
}

REGISTER_STATIC_SAVABLE(use_target_goal);

static void SP_target_goal(entity &ent)
{
	if (deathmatch)
	{
		// auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	ent.use = SAVABLE(use_target_goal);
	if (!st.noise)
		st.noise = "misc/secret.wav";
	ent.noise_index = gi.soundindex(st.noise);
	ent.svflags = SVF_NOCLIENT;
	level.total_goals++;
}

static REGISTER_ENTITY(TARGET_GOAL, target_goal);
#endif

//==========================================================


/*QUAKED target_explosion (1 0 0) (-8 -8 -8) (8 8 8)
Spawns an explosion temporary entity when used.

"delay"     wait this long before going off
"dmg"       how much radius damage should be done, defaults to 0
*/
static void target_explosion_explode(entity &self)
{
	gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1, self.origin).multicast(self.origin, MULTICAST_PHS);

	T_RadiusDamage(self, self.activator, (float)self.dmg, 0, (float)(self.dmg + 40), MOD_EXPLOSIVE);

	gtimef save = self.delay;
	self.delay = gtimef::zero();
	G_UseTargets(self, self.activator);
	self.delay = save;
}

REGISTER_STATIC_SAVABLE(target_explosion_explode);

static void use_target_explosion(entity &self, entity &, entity &cactivator)
{
	self.activator = cactivator;

	if (self.delay == gtimef::zero())
	{
		target_explosion_explode(self);
		return;
	}

	self.think = SAVABLE(target_explosion_explode);
	self.nextthink = duration_cast<gtime>(level.time + self.delay);
}

REGISTER_STATIC_SAVABLE(use_target_explosion);

static void SP_target_explosion(entity &ent)
{
	ent.use = SAVABLE(use_target_explosion);
	ent.svflags = SVF_NOCLIENT;
}

static REGISTER_ENTITY(TARGET_EXPLOSION, target_explosion);

//==========================================================

constexpr means_of_death MOD_EXIT { .self_kill_fmt = "{0} found a way out.\n" };

/*QUAKED target_changelevel (1 0 0) (-8 -8 -8) (8 8 8)
Changes level to "map" when fired
*/
static void use_target_changelevel(entity &self, entity &other, entity &cactivator)
{
	if (level.intermission_time != gtime::zero())
		return;     // already activated

#ifdef SINGLE_PLAYER
	if (!deathmatch && !coop)
	{
		if (itoe(1).health <= 0)
			return;
	}

	// if noexit, do a ton of damage to other
	if (deathmatch)
	{
#endif
		if (!(dmflags & DF_ALLOW_EXIT) && !other.is_world())
		{
			T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, 10 * other.max_health, 1000, { }, MOD_EXIT);
			return;
		}

		// if multiplayer, let everyone know who hit the exit
		if (cactivator.is_client)
			gi.bprintfmt(PRINT_HIGH, "{} exited the level.\n", cactivator.client.pers.netname);
#ifdef SINGLE_PLAYER
	}

	// if going to a new unit, clear cross triggers
	if (strstr(self.map, "*") != (size_t) -1)
		game.serverflags &= ~(SFL_CROSS_TRIGGER_MASK);
#endif

	BeginIntermission(self);
}

REGISTER_STATIC_SAVABLE(use_target_changelevel);

static void SP_target_changelevel(entity &ent)
{
	if (!ent.map)
	{
		gi.dprintfmt("{}: no map\n", ent);
		G_FreeEdict(ent);
		return;
	}

#ifdef SINGLE_PLAYER
	// ugly hack because *SOMEBODY* screwed up their map
	if (striequals(level.mapname, "fact1") && striequals(ent.map, "fact3"))
		ent.map = "fact3$secret1";
#endif

	ent.use = SAVABLE(use_target_changelevel);
	ent.svflags = SVF_NOCLIENT;
}

REGISTER_ENTITY(TARGET_CHANGELEVEL, target_changelevel);

//==========================================================

/*QUAKED target_splash (1 0 0) (-8 -8 -8) (8 8 8)
Creates a particle splash effect when used.

Set "sounds" to one of the following:
  1) sparks
  2) blue water
  3) brown water
  4) slime
  5) lava
  6) blood

"count" how many pixels in the splash
"dmg"   if set, does a radius damage at this location when it splashes
		useful for lava/sparks
*/

static void use_target_splash(entity &self, entity &, entity &cactivator)
{
	gi.ConstructMessage(svc_temp_entity, TE_SPLASH, (uint8_t) self.count, self.origin, vecdir { self.movedir }, (uint8_t) self.sounds).multicast(self.origin, MULTICAST_PVS);

	if (self.dmg)
		T_RadiusDamage(self, cactivator, (float)self.dmg, 0, (float)(self.dmg + 40));
}

REGISTER_STATIC_SAVABLE(use_target_splash);

static void SP_target_splash(entity &self)
{
	self.use = SAVABLE(use_target_splash);
	G_SetMovedir(self.angles, self.movedir);

	if (!self.count)
		self.count = 32;

	self.svflags = SVF_NOCLIENT;
}

static REGISTER_ENTITY(TARGET_SPLASH, target_splash);

//==========================================================

/*QUAKED target_spawner (1 0 0) (-8 -8 -8) (8 8 8)
Set target to the type of entity you want spawned.
Useful for spawning monsters and gibs in the factory levels.

For monsters:
	Set direction to the facing you want it to have.

For gibs:
	Set direction if you want it moving and
	speed how fast it should be moving otherwise it
	will just be dropped
*/

// from spawn.qc
bool ED_CallSpawn(entity &ent);

static void use_target_spawner(entity &self, entity &, entity &)
{
	entity &ent = G_Spawn();
	st.classname = self.target;
#ifdef THE_RECKONING
	ent.flags = self.flags;
#endif
	ent.origin = self.origin;
	ent.angles = self.angles;
	ED_CallSpawn(ent);
	gi.unlinkentity(ent);
	KillBox(ent);
	gi.linkentity(ent);
	if (self.speed)
		ent.velocity = self.movedir;

#ifdef GROUND_ZERO
	ent.renderfx |= RF_IR_VISIBLE;
#endif
}

REGISTER_STATIC_SAVABLE(use_target_spawner);

static void SP_target_spawner(entity &self)
{
	self.use = SAVABLE(use_target_spawner);
	self.svflags = SVF_NOCLIENT;
	if (self.speed)
	{
		G_SetMovedir(self.angles, self.movedir);
		self.movedir *= self.speed;
	}
}

static REGISTER_ENTITY(TARGET_SPAWNER, target_spawner);

//==========================================================

/*QUAKED target_blaster (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
Fires a blaster bolt in the set direction when triggered.

dmg     default is 15
speed   default is 1000
*/

static void use_target_blaster(entity &self, entity &, entity &)
{
	fire_blaster(self, self.origin, self.movedir, self.dmg, (int32_t)self.speed, EF_BLASTER, false);
	gi.sound(self, CHAN_VOICE, self.noise_index);
}

REGISTER_STATIC_SAVABLE(use_target_blaster);

static void SP_target_blaster(entity &self)
{
	self.use = SAVABLE(use_target_blaster);
	G_SetMovedir(self.angles, self.movedir);
	self.noise_index = gi.soundindex("weapons/laser2.wav");

	if (!self.dmg)
		self.dmg = 15;
	if (!self.speed)
		self.speed = 1000.f;

	self.svflags = SVF_NOCLIENT;
}

static REGISTER_ENTITY(TARGET_BLASTER, target_blaster);

#ifdef SINGLE_PLAYER
//==========================================================

/*QUAKED target_crosslevel_trigger (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Once this trigger is touched/used, any trigger_crosslevel_target with the same trigger number is automatically used when a level is started within the same unit.  It is OK to check multiple triggers.  Message, delay, target, and killtarget also work.
*/
static void trigger_crosslevel_trigger_use(entity &self, entity &, entity &)
{
	game.serverflags |= (cross_server_flags)self.spawnflags;
	G_FreeEdict(self);
}

REGISTER_STATIC_SAVABLE(trigger_crosslevel_trigger_use);

static void SP_target_crosslevel_trigger(entity &self)
{
	self.svflags = SVF_NOCLIENT;
	self.use = SAVABLE(trigger_crosslevel_trigger_use);
}

static REGISTER_ENTITY(TARGET_CROSSLEVEL_TRIGGER, target_crosslevel_trigger);

/*QUAKED target_crosslevel_target (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Triggered by a trigger_crosslevel elsewhere within a unit.  If multiple triggers are checked, all must be true.  Delay, target and
killtarget also work.

"delay"     delay before using targets if the trigger has been activated (default 1)
*/
static void target_crosslevel_target_think(entity &self)
{
	if (self.spawnflags == (spawn_flag)(game.serverflags & SFL_CROSS_TRIGGER_MASK & (cross_server_flags) self.spawnflags))
	{
		G_UseTargets(self, self);
		G_FreeEdict(self);
	}
}

REGISTER_STATIC_SAVABLE(target_crosslevel_target_think);

static void SP_target_crosslevel_target(entity &self)
{
	if (self.delay == gtime::zero())
		self.delay = 1s;
	self.svflags = SVF_NOCLIENT;

	self.think = SAVABLE(target_crosslevel_target_think);
	self.nextthink = duration_cast<gtime>(level.time + self.delay);
}

REGISTER_ENTITY(TARGET_CROSSLEVEL_TARGET, target_crosslevel_target);
#endif

//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON RED GREEN BLUE YELLOW ORANGE FAT
When triggered, fires a laser.  You can either set a target
or a direction.
*/

void target_laser_think(entity &self)
{
	entityref ignore;
	vector start;
	vector end;
	trace tr;
	vector point;
	vector last_movedir;
	int	count;

	if (self.spawnflags & LASER_BZZT)
		count = 8;
	else
		count = 4;

	if (self.enemy.has_value())
	{
		last_movedir = self.movedir;
		point = self.enemy->absbounds.center();
		self.movedir = point - self.origin;
		VectorNormalize(self.movedir);
		if (self.movedir != last_movedir)
			self.spawnflags |= LASER_BZZT;
	}

	ignore = self;
	start = self.origin;
	end = start + (2048 * self.movedir);
	while (1)
	{
#ifdef GROUND_ZERO
		if (self.spawnflags & LASER_STOPWINDOW)
			tr = gi.traceline(start, end, ignore, MASK_SHOT);
		else
#endif
			tr = gi.traceline(start, end, ignore, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

		if (tr.ent.is_world())
			break;

		// hurt it if we can
		if (tr.ent.takedamage && !(tr.ent.flags & FL_IMMUNE_LASER))
			T_Damage(tr.ent, self, self.activator, self.movedir, tr.endpos, vec3_origin, self.dmg, 1, { DAMAGE_ENERGY }, MOD_TARGET_LASER);

		// if we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(tr.ent.svflags & SVF_MONSTER) && !tr.ent.is_client
#ifdef GROUND_ZERO
			 && !(tr.ent.flags & FL_DAMAGEABLE)
#endif
			)
		{
			if (self.spawnflags & LASER_BZZT)
			{
				self.spawnflags &= ~LASER_BZZT;
				gi.ConstructMessage(svc_temp_entity, TE_LASER_SPARKS, (uint8_t) count, tr.endpos, vecdir { tr.normal }, (uint8_t) self.skinnum).multicast(tr.endpos, MULTICAST_PVS);
			}
			break;
		}

		ignore = tr.ent;
		start = tr.endpos;
	}

	self.old_origin = tr.endpos;

	self.nextthink = level.time + 1_hz;
}

REGISTER_SAVABLE(target_laser_think);

static void target_laser_on(entity &self)
{
	if (!self.activator.has_value())
		self.activator = self;
	self.spawnflags |= LASER_BZZT | LASER_ON;
	self.svflags &= ~SVF_NOCLIENT;
	target_laser_think(self);
}

static void target_laser_off(entity &self)
{
	self.spawnflags &= ~LASER_ON;
	self.svflags |= SVF_NOCLIENT;
	self.nextthink = gtime::zero();
}

static void target_laser_use(entity &self, entity &, entity &cactivator)
{
	self.activator = cactivator;
	if (self.spawnflags & LASER_ON)
		target_laser_off(self);
	else
		target_laser_on(self);
}

REGISTER_STATIC_SAVABLE(target_laser_use);

static void target_laser_start(entity &self)
{
	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_NOT;
	self.renderfx |= RF_BEAM | RF_TRANSLUCENT;
	self.modelindex = MODEL_WORLD;         // must be non-zero

	// set the beam diameter
	if (self.spawnflags & 64)
		self.frame = 16;
	else
		self.frame = 4;

	// set the color
	if (self.spawnflags & LASER_RED)
		self.skinnum = 0xf2f2f0f0;
	else if (self.spawnflags & LASER_GREEN)
		self.skinnum = 0xd0d1d2d3;
	else if (self.spawnflags & LASER_BLUE)
		self.skinnum = 0xf3f3f1f1;
	else if (self.spawnflags & LASER_YELLOW)
		self.skinnum = 0xdcdddedf;
	else if (self.spawnflags & LASER_ORANGE)
		self.skinnum = 0xe0e1e2e3;

	if (!self.enemy.has_value())
	{
		if (self.target)
		{
			entityref ent = G_FindFunc<&entity::targetname>(world, self.target, striequals);
			if (!ent.has_value())
				gi.dprintfmt("{}: {} is a bad target\n", self, self.target);
			self.enemy = ent;
		}
		else
			G_SetMovedir(self.angles, self.movedir);
	}

	self.use = SAVABLE(target_laser_use);
	self.think = SAVABLE(target_laser_think);

	if (!self.dmg)
		self.dmg = 1;

	self.bounds = bbox::sized(8.f);
	gi.linkentity(self);

	if (self.spawnflags & LASER_ON)
		target_laser_on(self);
	else
		target_laser_off(self);
}

REGISTER_STATIC_SAVABLE(target_laser_start);

static void SP_target_laser(entity &self)
{
	// let everything else get spawned before we start firing
	self.think = SAVABLE(target_laser_start);
	self.nextthink = level.time + 1s;
}

static REGISTER_ENTITY(TARGET_LASER, target_laser);
#ifdef SINGLE_PLAYER

//==========================================================

/*QUAKED target_lightramp (0 .5 .8) (-8 -8 -8) (8 8 8) TOGGLE
speed       How many seconds the ramping will take
message     two letters; starting lightlevel and ending lightlevel
*/

static void target_lightramp_think(entity &self)
{
	float diff = duration_cast<gtimef>(level.time - self.timestamp).count();

	gi.configstringfmt((config_string)(CS_LIGHTS + self.enemy->style), "{}", (char) ('a' + self.movedir.x + diff * self.movedir.z));

	if (diff < self.speed)
		self.nextthink = level.time + 100ms;
	else if (self.spawnflags & 1)
	{
		int temp = (int) self.movedir.x;
		self.movedir.x = self.movedir.y;
		self.movedir.y = (float) temp;
		self.movedir.z *= -1;
	}
}

REGISTER_STATIC_SAVABLE(target_lightramp_think);

static void target_lightramp_use(entity &self, entity &, entity &)
{
	if (!self.enemy.has_value())
	{
		// check all the targets
		for (entity &e : G_IterateFunc<&entity::targetname>(self.target, striequals))
		{
			if (e.type != ET_LIGHT)
				gi.dprintfmt("{}: target \"{}\" ({}) is not a light\n", self, self.target, e);
			else
				self.enemy = e;
		}

		if (!self.enemy.has_value())
		{
			gi.dprintfmt("{}: target \"{}\" not found\n", self, self.target);
			G_FreeEdict(self);
			return;
		}
	}

	self.timestamp = level.time;
	target_lightramp_think(self);
}

REGISTER_STATIC_SAVABLE(target_lightramp_use);

static void SP_target_lightramp(entity &self)
{
	if (!self.message || strlen(self.message) != 2 || self.message[0] < 'a' || self.message[0] > 'z' || self.message[1] < 'a' || self.message[1] > 'z' || self.message[0] == self.message[1])
	{
		gi.dprintfmt("{}: bad ramp \"{}\"\n", self, self.message);
		G_FreeEdict(self);
		return;
	}

	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self.target)
	{
		gi.dprintfmt("{}: no target\n", self);
		G_FreeEdict(self);
		return;
	}

	self.svflags |= SVF_NOCLIENT;
	self.use = SAVABLE(target_lightramp_use);
	self.think = SAVABLE(target_lightramp_think);

	self.movedir.x = (float)(self.message[0] - 'a');
	self.movedir.y = (float)(self.message[1] - 'a');
	self.movedir.z = (self.movedir.y - self.movedir.x) / self.speed;
}

static REGISTER_ENTITY(TARGET_LIGHTRAMP, target_lightramp);
#endif
//==========================================================

/*QUAKED target_earthquake (1 0 0) (-8 -8 -8) (8 8 8)
When triggered, this initiates a level-wide earthquake.
All players and monsters are affected.
"speed"     severity of the quake (default:200)
"count"     duration of the quake (default:5)
*/

static void target_earthquake_think(entity &self)
{
	if (
#ifdef GROUND_ZERO
		self.noise_index &&
#endif
		self.last_move_time < level.time)
	{
		gi.positioned_sound(self.origin, self, self.noise_index, ATTN_NONE);
		self.last_move_time = level.time + 500ms;
	}

	for (uint32_t i = 1; i < num_entities; i++)
	{
		entity &e = itoe(i);
		if (!e.inuse)
			continue;
		if (!e.is_client)
			continue;
		if (!e.groundentity.has_value())
			continue;

		e.groundentity = null_entity;
		e.velocity.x += crandom(150.f);
		e.velocity.y += crandom(150.f);
		e.velocity.z = self.speed * (100.0f / e.mass);
	}

	if (level.time < self.timestamp)
		self.nextthink = level.time + 1_hz;
}

REGISTER_STATIC_SAVABLE(target_earthquake_think);

static void target_earthquake_use(entity &self, entity &, entity &cactivator)
{
	self.timestamp = level.time + seconds(self.count);
	self.nextthink = level.time + 1_hz;
	self.activator = cactivator;
	self.last_move_time = gtime::zero();
}

REGISTER_STATIC_SAVABLE(target_earthquake_use);

static void SP_target_earthquake(entity &self)
{
	if (!self.targetname)
		gi.dprintfmt("{}: no targetname\n", self);

	if (!self.count)
		self.count = 5;

	if (!self.speed)
		self.speed = 200.f;

	self.svflags |= SVF_NOCLIENT;
	self.think = SAVABLE(target_earthquake_think);
	self.use = SAVABLE(target_earthquake_use);

#ifdef GROUND_ZERO
	if (!(self.spawnflags & 1))
#endif
		self.noise_index = gi.soundindex("world/quake.wav");
}

static REGISTER_ENTITY(TARGET_EARTHQUAKE, target_earthquake);