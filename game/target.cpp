#include "../lib/types.h"
#include "../lib/entity.h"
#include "../lib/gi.h"
#include "game.h"
#include "combat.h"
#include "util.h"
#include "hud.h"
#include "gweapon.h"
#include "spawn.h"

/*QUAKED target_temp_entity (1 0 0) (-8 -8 -8) (8 8 8)
Fire an origin based temp entity event to the clients.
"style"     type byte
*/
static void Use_Target_Tent(entity &ent, entity &, entity &)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte((uint8_t)ent.g.style);
	gi.WritePosition(ent.s.origin);
	gi.multicast(ent.s.origin, MULTICAST_PVS);
}

static void SP_target_temp_entity(entity &ent)
{
	ent.g.use = Use_Target_Tent;
}

REGISTER_ENTITY(target_temp_entity, ET_TARGET_TEMP_ENTITY);

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

	if (ent.g.spawnflags & (SPEAKER_LOOPED_ON | SPEAKER_LOOPED_OFF))
	{
		// looping sound toggles
		if (ent.s.sound)
			ent.s.sound = SOUND_NONE;   // turn it off
		else
			ent.s.sound = ent.g.noise_index;    // start it
	}
	else
	{
		// normal sound
		if (ent.g.spawnflags & SPEAKER_RELIABLE)
			chan = CHAN_VOICE | CHAN_RELIABLE;
		else
			chan = CHAN_VOICE;

		// use a positioned_sound, because this entity won't normally be
		// sent to any clients because it is invisible
		gi.positioned_sound(ent.s.origin, ent, chan, ent.g.noise_index, ent.g.volume, ent.g.attenuation, 0);
	}
}

static void SP_target_speaker(entity &ent)
{
	string	buffer = st.noise;

	if (!buffer)
	{
		gi.dprintf("target_speaker with no noise set at %s\n", vtos(ent.s.origin).ptr());
		return;
	}

	if (strstr(st.noise, ".wav") == -1)
		buffer = va("%s.wav", st.noise.ptr());

	ent.g.noise_index = gi.soundindex(buffer);

	if (!ent.g.volume)
		ent.g.volume = 1.0f;

	if (!ent.g.attenuation)
		ent.g.attenuation = ATTN_NORM;
	else if (ent.g.attenuation == -1)    // use -1 so 0 defaults to 1
		ent.g.attenuation = ATTN_NONE;

	// check for prestarted looping sound
	if (ent.g.spawnflags & SPEAKER_LOOPED_ON)
		ent.s.sound = ent.g.noise_index;

	ent.g.use = Use_Target_Speaker;

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity(ent);
}

REGISTER_ENTITY(target_speaker, ET_TARGET_SPEAKER);

#ifdef SINGLE_PLAYER
//==========================================================

static void(entity ent, entity other, entity cactivator) Use_Target_Help =
{
	if (ent.spawnflags & 1)
		game.helpmessage1 = ent.message;
	else
		game.helpmessage2 = ent.message;

	game.helpchanged++;
}

/*QUAKED target_help (1 0 1) (-16 -16 -24) (16 16 24) help1
When fired, the "message" key becomes the current personal computer string, and the message light will be set on all clients status bars.
*/
API_FUNC static void(entity ent) SP_target_help =
{
	if (deathmatch.intVal)
	{
		// auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	if (!ent.message)
	{
		gi.dprintf("%s with no message at %s\n", ent.classname, vtos(ent.s.origin));
		G_FreeEdict(ent);
		return;
	}
	ent.use = Use_Target_Help;
}

//==========================================================

/*QUAKED target_secret (1 0 1) (-8 -8 -8) (8 8 8)
Counts a secret found.
These are single use targets.
*/
static void(entity ent, entity other, entity cactivator) use_target_secret =
{
	gi.sound(ent, CHAN_VOICE, ent.noise_index, 1, ATTN_NORM, 0);

	level.found_secrets++;

	G_UseTargets(ent, cactivator);
	G_FreeEdict(ent);
}

API_FUNC static void(entity ent) SP_target_secret =
{
	if (deathmatch.intVal)
	{
		// auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	ent.use = use_target_secret;
	if (!st.noise)
		st.noise = "misc/secret.wav";
	ent.noise_index = gi.soundindex(st.noise);
	ent.svflags = SVF_NOCLIENT;
	level.total_secrets++;
	// map bug hack
	if (striequals(level.mapname, "mine3") && ent.s.origin[0] == 280 && ent.s.origin[1] == -2048 && ent.s.origin[2] == -624)
		ent.message = "You have found a secret area.";
}

//==========================================================

/*QUAKED target_goal (1 0 1) (-8 -8 -8) (8 8 8)
Counts a goal completed.
These are single use targets.
*/
static void(entity ent, entity other, entity cactivator) use_target_goal =
{
	gi.sound(ent, CHAN_VOICE, ent.noise_index, 1, ATTN_NORM, 0);

	level.found_goals++;

	if (level.found_goals == level.total_goals)
		gi.configstring(CS_CDTRACK, "0");

	G_UseTargets(ent, cactivator);
	G_FreeEdict(ent);
}

API_FUNC static void(entity ent) SP_target_goal =
{
	if (deathmatch.intVal)
	{
		// auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	ent.use = use_target_goal;
	if (!st.noise)
		st.noise = "misc/secret.wav";
	ent.noise_index = gi.soundindex(st.noise);
	ent.svflags = SVF_NOCLIENT;
	level.total_goals++;
}
#endif

//==========================================================


/*QUAKED target_explosion (1 0 0) (-8 -8 -8) (8 8 8)
Spawns an explosion temporary entity when used.

"delay"     wait this long before going off
"dmg"       how much radius damage should be done, defaults to 0
*/
static void target_explosion_explode(entity &self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self.s.origin);
	gi.multicast(self.s.origin, MULTICAST_PHS);

	T_RadiusDamage(self, self.g.activator, (float)self.g.dmg, 0, (float)(self.g.dmg + 40), MOD_EXPLOSIVE);

	float save = self.g.delay;
	self.g.delay = 0;
	G_UseTargets(self, self.g.activator);
	self.g.delay = save;
}

static void use_target_explosion(entity &self, entity &, entity &cactivator)
{
	self.g.activator = cactivator;

	if (!self.g.delay)
	{
		target_explosion_explode(self);
		return;
	}

	self.g.think = target_explosion_explode;
	self.g.nextthink = level.framenum + (gtime)(self.g.delay * BASE_FRAMERATE);
}

static void SP_target_explosion(entity &ent)
{
	ent.g.use = use_target_explosion;
	ent.svflags = SVF_NOCLIENT;
}

REGISTER_ENTITY(target_explosion, ET_TARGET_EXPLOSION);

//==========================================================

/*QUAKED target_changelevel (1 0 0) (-8 -8 -8) (8 8 8)
Changes level to "map" when fired
*/
static void use_target_changelevel(entity &self, entity &other, entity &cactivator)
{
	if (level.intermission_framenum)
		return;     // already activated

#ifdef SINGLE_PLAYER
	if (!deathmatch.intVal && !coop.intVal)
	{
		if (itoe(1).health <= 0)
			return;
	}

	// if noexit, do a ton of damage to other
	if (deathmatch.intVal)
	{
#endif
		if (!((dm_flags)dmflags & DF_ALLOW_EXIT) && other != world)
		{
			T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, 10 * other.g.max_health, 1000, DAMAGE_NONE, MOD_EXIT);
			return;
		}

		// if multiplayer, let everyone know who hit the exit
		if (cactivator.is_client())
			gi.bprintf(PRINT_HIGH, "%s exited the level.\n", cactivator.client->g.pers.netname.ptr());
#ifdef SINGLE_PLAYER
	}

	// if going to a new unit, clear cross triggers
	if (strstr(self.map, "*") != -1)
		game.serverflags &= ~(SFL_CROSS_TRIGGER_MASK);
#endif

	BeginIntermission(self);
}

static void SP_target_changelevel(entity &ent)
{
	if (!ent.g.map)
	{
		gi.dprintf("target_changelevel with no map at %s\n", vtos(ent.s.origin).ptr());
		G_FreeEdict(ent);
		return;
	}

#ifdef SINGLE_PLAYER
	// ugly hack because *SOMEBODY* screwed up their map
	if (striequals(level.mapname, "fact1") && striequals(ent.map, "fact3"))
		ent.map = "fact3$secret1";
#endif

	ent.g.use = use_target_changelevel;
	ent.svflags = SVF_NOCLIENT;
}

REGISTER_ENTITY(target_changelevel, ET_TARGET_CHANGELEVEL);

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
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPLASH);
	gi.WriteByte((uint8_t)self.g.count);
	gi.WritePosition(self.s.origin);
	gi.WriteDir(self.g.movedir);
	gi.WriteByte((uint8_t)self.g.sounds);
	gi.multicast(self.s.origin, MULTICAST_PVS);

	if (self.g.dmg)
		T_RadiusDamage(self, cactivator, (float)self.g.dmg, 0, (float)(self.g.dmg + 40), MOD_SPLASH);
}

static void SP_target_splash(entity &self)
{
	self.g.use = use_target_splash;
	G_SetMovedir(self.s.angles, self.g.movedir);

	if (!self.g.count)
		self.g.count = 32;

	self.svflags = SVF_NOCLIENT;
}

REGISTER_ENTITY(target_splash, ET_TARGET_SPLASH);

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
	st.classname = self.g.target;
#ifdef THE_RECKONING
	ent.flags = self.flags;
#endif
	ent.s.origin = self.s.origin;
	ent.s.angles = self.s.angles;
	ED_CallSpawn(ent);
	gi.unlinkentity(ent);
	KillBox(ent);
	gi.linkentity(ent);
	if (self.g.speed)
		ent.g.velocity = self.g.movedir;

#ifdef GROUND_ZERO
	ent.s.renderfx |= RF_IR_VISIBLE;
#endif
}

static void SP_target_spawner(entity &self)
{
	self.g.use = use_target_spawner;
	self.svflags = SVF_NOCLIENT;
	if (self.g.speed)
	{
		G_SetMovedir(self.s.angles, self.g.movedir);
		self.g.movedir *= self.g.speed;
	}
}

REGISTER_ENTITY(target_spawner, ET_TARGET_SPAWNER);

//==========================================================

/*QUAKED target_blaster (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
Fires a blaster bolt in the set direction when triggered.

dmg     default is 15
speed   default is 1000
*/

static void use_target_blaster(entity &self, entity &, entity &)
{
	fire_blaster(self, self.s.origin, self.g.movedir, self.g.dmg, (int32_t)self.g.speed, EF_BLASTER, MOD_TARGET_BLASTER, false);
	gi.sound(self, CHAN_VOICE, self.g.noise_index, 1, ATTN_NORM, 0);
}

static void SP_target_blaster(entity &self)
{
	self.g.use = use_target_blaster;
	G_SetMovedir(self.s.angles, self.g.movedir);
	self.g.noise_index = gi.soundindex("weapons/laser2.wav");

	if (!self.g.dmg)
		self.g.dmg = 15;
	if (!self.g.speed)
		self.g.speed = 1000.f;

	self.svflags = SVF_NOCLIENT;
}

REGISTER_ENTITY(target_blaster, ET_TARGET_BLASTER);

#ifdef SINGLE_PLAYER
//==========================================================

/*QUAKED target_crosslevel_trigger (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Once this trigger is touched/used, any trigger_crosslevel_target with the same trigger number is automatically used when a level is started within the same unit.  It is OK to check multiple triggers.  Message, delay, target, and killtarget also work.
*/
static void(entity self, entity other, entity cactivator) trigger_crosslevel_trigger_use =
{
	game.serverflags |= self.spawnflags;
	G_FreeEdict(self);
}

API_FUNC static void(entity self) SP_target_crosslevel_trigger =
{
	self.svflags = SVF_NOCLIENT;
	self.use = trigger_crosslevel_trigger_use;
}

/*QUAKED target_crosslevel_target (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Triggered by a trigger_crosslevel elsewhere within a unit.  If multiple triggers are checked, all must be true.  Delay, target and
killtarget also work.

"delay"     delay before using targets if the trigger has been activated (default 1)
*/
static void(entity self) target_crosslevel_target_think =
{
	if (self.spawnflags == (game.serverflags & SFL_CROSS_TRIGGER_MASK & self.spawnflags)) {
		G_UseTargets(self, self);
		G_FreeEdict(self);
	}
}

API_FUNC static void(entity self) SP_target_crosslevel_target =
{
	if (!self.delay)
		self.delay = 1f;
	self.svflags = SVF_NOCLIENT;

	self.think = target_crosslevel_target_think;
	self.nextthink = level.framenum + (int)(self.delay * BASE_FRAMERATE);
}
#endif

//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON RED GREEN BLUE YELLOW ORANGE FAT
When triggered, fires a laser.  You can either set a target
or a direction.
*/

constexpr spawn_flag LASER_ON = (spawn_flag)1;
constexpr spawn_flag LASER_RED = (spawn_flag)2;
constexpr spawn_flag LASER_GREEN = (spawn_flag)4;
constexpr spawn_flag LASER_BLUE = (spawn_flag)8;
constexpr spawn_flag LASER_YELLOW = (spawn_flag)16;
constexpr spawn_flag LASER_ORANGE = (spawn_flag)32;
constexpr spawn_flag LASER_FAT = (spawn_flag)64;
constexpr spawn_flag LASER_STOPWINDOW = (spawn_flag)128;
constexpr spawn_flag LASER_BZZT = (spawn_flag)0x80000000;

void target_laser_think(entity &self)
{
	entityref ignore;
	vector start;
	vector end;
	trace tr;
	vector point;
	vector last_movedir;
	int	count;

	if (self.g.spawnflags & LASER_BZZT)
		count = 8;
	else
		count = 4;

	if (self.g.enemy.has_value())
	{
		last_movedir = self.g.movedir;
		point = self.g.enemy->absmin + (0.5f * self.g.enemy->size);
		self.g.movedir = point - self.s.origin;
		VectorNormalize(self.g.movedir);
		if (self.g.movedir != last_movedir)
			self.g.spawnflags |= LASER_BZZT;
	}

	ignore = self;
	start = self.s.origin;
	end = start + (2048 * self.g.movedir);
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
		if (tr.ent.g.takedamage && !(tr.ent.g.flags & FL_IMMUNE_LASER))
			T_Damage(tr.ent, self, self.g.activator, self.g.movedir, tr.endpos, vec3_origin, self.g.dmg, 1, DAMAGE_ENERGY, MOD_TARGET_LASER);

		// if we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(tr.ent.svflags & SVF_MONSTER) && !tr.ent.is_client()
#ifdef GROUND_ZERO
			 && !(tr.ent.svflags & SVF_DAMAGEABLE)
#endif
			)
		{
			if (self.g.spawnflags & LASER_BZZT)
			{
				self.g.spawnflags &= ~LASER_BZZT;
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte((uint8_t)count);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.normal);
				gi.WriteByte((uint8_t)self.s.skinnum);
				gi.multicast(tr.endpos, MULTICAST_PVS);
			}
			break;
		}

		ignore = tr.ent;
		start = tr.endpos;
	}

	self.s.old_origin = tr.endpos;

	self.g.nextthink = level.framenum + 1;
}

static void target_laser_on(entity &self)
{
	if (!self.g.activator.has_value())
		self.g.activator = self;
	self.g.spawnflags |= LASER_BZZT | LASER_ON;
	self.svflags &= ~SVF_NOCLIENT;
	target_laser_think(self);
}

static void target_laser_off(entity &self)
{
	self.g.spawnflags &= ~LASER_ON;
	self.svflags |= SVF_NOCLIENT;
	self.g.nextthink = 0;
}

static void target_laser_use(entity &self, entity &, entity &cactivator)
{
	self.g.activator = cactivator;
	if (self.g.spawnflags & LASER_ON)
		target_laser_off(self);
	else
		target_laser_on(self);
}

static void target_laser_start(entity &self)
{
	self.g.movetype = MOVETYPE_NONE;
	self.solid = SOLID_NOT;
	self.s.renderfx |= RF_BEAM | RF_TRANSLUCENT;
	self.s.modelindex = MODEL_WORLD;         // must be non-zero

	// set the beam diameter
	if (self.g.spawnflags & 64)
		self.s.frame = 16;
	else
		self.s.frame = 4;

	// set the color
	if (self.g.spawnflags & LASER_RED)
		self.s.skinnum = 0xf2f2f0f0;
	else if (self.g.spawnflags & LASER_GREEN)
		self.s.skinnum = 0xd0d1d2d3;
	else if (self.g.spawnflags & LASER_BLUE)
		self.s.skinnum = 0xf3f3f1f1;
	else if (self.g.spawnflags & LASER_YELLOW)
		self.s.skinnum = 0xdcdddedf;
	else if (self.g.spawnflags & LASER_ORANGE)
		self.s.skinnum = 0xe0e1e2e3;

	if (!self.g.enemy.has_value())
	{
		if (self.g.target)
		{
			entityref ent = G_FindFunc(world, g.targetname, self.g.target, striequals);
			if (!ent.has_value())
				gi.dprintf("%s at %s: %s is a bad target\n", st.classname.ptr(), vtos(self.s.origin).ptr(), self.g.target.ptr());
			self.g.enemy = ent;
		}
		else
			G_SetMovedir(self.s.angles, self.g.movedir);
	}

	self.g.use = target_laser_use;
	self.g.think = target_laser_think;

	if (!self.g.dmg)
		self.g.dmg = 1;

	self.mins = { -8, -8, -8 };
	self.maxs = { 8, 8, 8 };
	gi.linkentity(self);

	if (self.g.spawnflags & LASER_ON)
		target_laser_on(self);
	else
		target_laser_off(self);
}

static void SP_target_laser(entity &self)
{
	// let everything else get spawned before we start firing
	self.g.think = target_laser_start;
	self.g.nextthink = level.framenum + 1 * BASE_FRAMERATE;
}

REGISTER_ENTITY(target_laser, ET_TARGET_LASER);
#ifdef SINGLE_PLAYER

//==========================================================

/*QUAKED target_lightramp (0 .5 .8) (-8 -8 -8) (8 8 8) TOGGLE
speed       How many seconds the ramping will take
message     two letters; starting lightlevel and ending lightlevel
*/

static void(entity self) target_lightramp_think =
{
	float   diff = (level.framenum - self.timestamp) * FRAMETIME;

	string s = va("%c", (int)('a' + self.movedir.x + diff * self.movedir.z));
	gi.configstring(CS_LIGHTS + self.enemy.style, s);

	if (diff < self.speed)
		self.nextthink = level.framenum + 1;
	else if (self.spawnflags & 1)
	{
		int temp = (int)self.movedir.x;
		self.movedir.x = self.movedir.y;
		self.movedir.y = (float)temp;
		self.movedir.z *= -1;
	}
}

static void(entity self, entity other, entity cactivator) target_lightramp_use =
{
	if (!self.enemy)
	{
		// check all the targets
		entity e = world;

		while ((e = G_Find(e, targetname, self.target)))
		{
			if (e.classname != "light")
			{
				gi.dprintf("%s at %s ", self.classname, vtos(self.s.origin));
				gi.dprintf("target %s (%s at %s) is not a light\n", self.target, e.classname, vtos(e.s.origin));
			}
			else
				self.enemy = e;
		}

		if (!self.enemy)
		{
			gi.dprintf("%s target %s not found at %s\n", self.classname, self.target, vtos(self.s.origin));
			G_FreeEdict(self);
			return;
		}
	}

	self.timestamp = level.framenum;
	target_lightramp_think(self);
}

API_FUNC static void(entity self) SP_target_lightramp =
{
	if (!self.message || strlen(self.message) != 2 || strat(self.message, 0) < 'a' || strat(self.message, 0) > 'z' || strat(self.message, 1) < 'a' || strat(self.message, 1) > 'z' || strat(self.message, 0) == strat(self.message, 1))
	{
		gi.dprintf("target_lightramp has bad ramp (%s) at %s\n", self.message, vtos(self.s.origin));
		G_FreeEdict(self);
		return;
	}

	if (deathmatch.intVal)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self.target) {
		gi.dprintf("%s with no target at %s\n", st.classname, vtos(self.s.origin));
		G_FreeEdict(self);
		return;
	}

	self.svflags |= SVF_NOCLIENT;
	self.use = target_lightramp_use;
	self.think = target_lightramp_think;

	self.movedir.x = (float)(strat(self.message, 0) - 'a');
	self.movedir.y = (float)(strat(self.message, 1) - 'a');
	self.movedir.z = (self.movedir.y - self.movedir.x) / self.speed;
}
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
		self.g.last_move_framenum < level.framenum)
	{
		gi.positioned_sound(self.s.origin, self, CHAN_AUTO, self.g.noise_index, 1.0f, ATTN_NONE, 0);
		self.g.last_move_framenum = level.framenum + (gtime)(0.5f * BASE_FRAMERATE);
	}

	for (uint32_t i = 1; i < num_entities; i++)
	{
		entity &e = itoe(i);
		if (!e.inuse)
			continue;
		if (!e.is_client())
			continue;
		if (!e.g.groundentity.has_value())
			continue;

		e.g.groundentity = null_entity;
		e.g.velocity.x += random(-150.f, 150.f);
		e.g.velocity.y += random(-150.f, 150.f);
		e.g.velocity.z = self.g.speed * (100.0f / e.g.mass);
	}

	if (level.framenum < self.g.timestamp)
		self.g.nextthink = level.framenum + 1;
}

static void target_earthquake_use(entity &self, entity &, entity &cactivator)
{
	self.g.timestamp = level.framenum + self.g.count * BASE_FRAMERATE;
	self.g.nextthink = level.framenum + 1;
	self.g.activator = cactivator;
	self.g.last_move_framenum = 0;
}

static void SP_target_earthquake(entity &self)
{
	if (!self.g.targetname)
		gi.dprintf("untargeted %s at %s\n", st.classname.ptr(), vtos(self.s.origin).ptr());

	if (!self.g.count)
		self.g.count = 5;

	if (!self.g.speed)
		self.g.speed = 200.f;

	self.svflags |= SVF_NOCLIENT;
	self.g.think = target_earthquake_think;
	self.g.use = target_earthquake_use;

#ifdef GROUND_ZERO
	if(!(self.spawnflags & 1))
#endif
		self.g.noise_index = gi.soundindex("world/quake.wav");
}

REGISTER_ENTITY(target_earthquake, ET_TARGET_EARTHQUAKE);