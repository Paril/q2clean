#include "lib/std.h"
#include "config.h"
#include "entity.h"
#include "misc.h"
#include "game.h"
#include "spawn.h"

#include "lib/gi.h"
#include "game/util.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "combat.h"
#include <ctime>

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

static REGISTER_ENTITY_NOSPAWN(FUNC_GROUP, func_group);

//=====================================================
static void Use_Areaportal(entity &ent, entity &, entity &)
{
	ent.count ^= 1;        // toggle state
	gi.SetAreaPortalState(ent.style, ent.count);
}

REGISTER_STATIC_SAVABLE(Use_Areaportal);

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
static void SP_func_areaportal(entity &ent)
{
	ent.use = SAVABLE(Use_Areaportal);
	ent.count = 0;     // always start closed;
}

REGISTER_ENTITY(FUNC_AREAPORTAL, func_areaportal);

//=====================================================

void ClipGibVelocity(entity &ent)
{
	ent.velocity.x = clamp(-300.f, ent.velocity.x, 300.f);
	ent.velocity.y = clamp(-300.f, ent.velocity.y, 300.f);
	ent.velocity.z = clamp(200.f, ent.velocity.z, 500.f); // always some upwards
}

/*
=================
gibs
=================
*/
static void gib_think(entity &self)
{
	self.frame++;
	self.nextthink = level.time + 100ms;

	if (self.frame == 10)
	{
		self.think = SAVABLE(G_FreeEdict);
		self.nextthink = level.time + random(8s, 18s);
	}
}

REGISTER_STATIC_SAVABLE(gib_think);

void gib_touch(entity &self, entity &, vector normal, const surface &)
{
	if (!self.groundentity.has_value())
		return;

	self.touch = nullptr;

	if (normal)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/fhit3.wav"));

		vector normal_angles = vectoangles(normal);
		vector right;
		AngleVectors(normal_angles, nullptr, &right, nullptr);
		self.angles = vectoangles(right);

		if (self.modelindex == sm_meat_index)
		{
			self.frame++;
			self.think = SAVABLE(gib_think);
			self.nextthink = level.time + 100ms;
		}
	}
}

REGISTER_SAVABLE(gib_touch);

void gib_die(entity &self, entity &, entity &, int32_t, vector)
{
	G_FreeEdict(self);
}

REGISTER_SAVABLE(gib_die);

entity &ThrowGib(entity &self, stringlit gibname, int32_t damage, gib_type type)
{
	entity &gib = G_Spawn();

	vector sz = self.size * 0.5f;
	vector origin = self.absbounds.mins + sz;
	gib.origin = origin + randomv(-sz, sz);

	gi.setmodel(gib, gibname);
	gib.solid = SOLID_NOT;
	gib.effects |= EF_GIB;
	gib.flags |= FL_NO_KNOCKBACK;
	gib.takedamage = true;
	gib.die = SAVABLE(gib_die);

	float vscale;

	if (type == GIB_ORGANIC)
	{
		gib.movetype = MOVETYPE_TOSS;
		gib.touch = SAVABLE(gib_touch);
		vscale = 0.5f;
	}
	else
	{
		gib.movetype = MOVETYPE_BOUNCE;
		vscale = 1.0f;
	}

	gib.velocity = self.velocity + (vscale * VelocityForDamage(damage));
	ClipGibVelocity(gib);
	gib.avelocity = crandomv({ 600, 600, 600 });
	gib.angles = randomv({ 360, 360, 360 });

	gib.think = SAVABLE(G_FreeEdict);
	gib.nextthink = level.time + random(10s, 20s);

	gi.linkentity(gib);

	return gib;
}

void ThrowHead(entity &self, stringlit gibname, int32_t damage, gib_type type)
{
	self.skinnum = 0;
	self.frame = 0;
	self.bounds = bbox_point;
	
	self.modelindex2 = MODEL_NONE;
	gi.setmodel(self, gibname);
	self.solid = SOLID_NOT;
	self.effects |= EF_GIB;
	self.effects &= ~EF_FLIES;
	self.sound = SOUND_NONE;
	self.flags |= FL_NO_KNOCKBACK;
	self.svflags &= ~SVF_MONSTER;
	self.takedamage = true;
	self.die = SAVABLE(gib_die);

	float vscale;

	if (type == GIB_ORGANIC)
	{
		self.movetype = MOVETYPE_TOSS;
		self.touch = SAVABLE(gib_touch);
		vscale = 0.5f;
	}
	else
	{
		self.movetype = MOVETYPE_BOUNCE;
		vscale = 1.0f;
	}

	self.velocity += (vscale * VelocityForDamage(damage));
	ClipGibVelocity(self);

	self.avelocity[YAW] = crandom(600.f);

	self.think = SAVABLE(G_FreeEdict);
	self.nextthink = level.time + random(10s, 20s);

	gi.linkentity(self);
}

void ThrowClientHead(entity &self, int32_t damage)
{
	stringlit	gibname;

	if (Q_rand_bool())
	{
		gibname = "models/objects/gibs/head2/tris.md2";
		self.skinnum = 1;        // second skin is player
	}
	else
	{
		gibname = "models/objects/gibs/skull/tris.md2";
		self.skinnum = 0;
	}

	self.origin[2] += 32;
	self.frame = 0;
	gi.setmodel(self, gibname);
	self.bounds = {
		.mins = { -16, -16, 0 },
		.maxs = { 16, 16, 16 }
	};

	self.takedamage = false;
	self.solid = SOLID_NOT;
	self.effects = EF_GIB;
	self.sound = SOUND_NONE;
	self.flags |= FL_NO_KNOCKBACK;

	self.movetype = MOVETYPE_BOUNCE;
	self.velocity += VelocityForDamage(damage);

	if (self.is_client)
	{ // bodies in the queue don't have a client anymore
		self.client.anim_priority = ANIM_DEATH;
		self.client.anim_end = self.frame;
	}
	else
	{
		self.think = nullptr;
		self.nextthink = gtime::zero();
	}

	gi.linkentity(self);
}

/*
=================
debris
=================
*/
void ThrowDebris(entity &self, stringlit modelname, float speed, vector origin)
{
	entity &chunk = G_Spawn();
	chunk.origin = origin;
	gi.setmodel(chunk, modelname);
	vector v = { crandom(100.f), crandom(100.f), random(200.f) };
	chunk.velocity = self.velocity + (speed * v);
	chunk.movetype = MOVETYPE_BOUNCE;
	chunk.solid = SOLID_NOT;
	chunk.avelocity = crandomv({ 600, 600, 600 });
	chunk.angles = randomv({ 360, 360, 360 });
	chunk.think = SAVABLE(G_FreeEdict);
	chunk.nextthink = level.time + random(5s, 10s);
	chunk.frame = 0;
	chunk.flags = FL_NONE;
	chunk.takedamage = true;
	chunk.die = SAVABLE(gib_die);
	gi.linkentity(chunk);
}

/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
Target: next path corner
Pathtarget: gets used when an entity that has
	this path_corner targeted touches it
*/
constexpr spawn_flag CORNER_TELEPORT = (spawn_flag)1;

static void path_corner_touch(entity &self, entity &other, vector , const surface &)
{
	vector	v;

	if (other.movetarget != self)
		return;

	if (other.enemy.has_value())
		return;

	if (self.pathtarget)
	{
		string savetarget = self.target;
		self.target = self.pathtarget;
		G_UseTargets(self, other);
		self.target = savetarget;
	}

	entityref next;

	if (self.target)
		next = G_PickTarget(self.target);

	if (next.has_value() && (next->spawnflags & CORNER_TELEPORT))
	{
		v = next->origin;
		v.z += next->bounds.mins.z;
		v.z -= other.bounds.mins.z;
		other.origin = v;
		next = G_PickTarget(next->target);
		other.event = EV_OTHER_TELEPORT;
	}

	other.goalentity = other.movetarget = next;

#ifdef SINGLE_PLAYER
	if (self.wait != gtimef::zero())
	{
		other.monsterinfo.pause_time = duration_cast<gtime>(level.time + self.wait);
		other.monsterinfo.stand(other);
		return;
	}

	if (!other.movetarget.has_value())
	{
		other.monsterinfo.pause_time = gtime::max();
		other.monsterinfo.stand(other);
	}
	else
	{
		v = other.goalentity->origin - other.origin;
		other.ideal_yaw = vectoyaw(v);
	}
#endif
}

REGISTER_STATIC_SAVABLE(path_corner_touch);

static void SP_path_corner(entity &self)
{
	if (!self.targetname)
	{
		gi.dprintfmt("{}: no targetname\n", self);
		G_FreeEdict(self);
		return;
	}

	self.solid = SOLID_TRIGGER;
	self.touch = SAVABLE(path_corner_touch);
	self.bounds = bbox::sized(8.f);
	self.svflags |= SVF_NOCLIENT;
	gi.linkentity(self);
}

REGISTER_ENTITY(PATH_CORNER, path_corner);

#ifdef SINGLE_PLAYER
/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
static void point_combat_touch(entity &self, entity &other, vector, const surface &)
{
	entityref cactivator;

	if (other.movetarget != self)
		return;

	if (self.target)
	{
		other.target = self.target;
		other.goalentity = other.movetarget = G_PickTarget(other.target);
		if (!other.goalentity.has_value())
		{
			gi.dprintfmt("{}: target {} does not exist\n", self, self.target);
			other.movetarget = self;
		}
		self.target = nullptr;
	}
	else if ((self.spawnflags & 1) && !(other.flags & (FL_SWIM | FL_FLY)))
	{
		other.monsterinfo.pause_time = gtime::max();
		other.monsterinfo.aiflags |= AI_STAND_GROUND;
		other.monsterinfo.stand(other);
	}

	if (other.movetarget == self)
	{
		other.target = nullptr;
		other.movetarget = null_entity;
		other.goalentity = other.enemy;
		other.monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self.pathtarget)
	{
		string savetarget = self.target;
		self.target = self.pathtarget;
		if (other.enemy.has_value() && other.enemy->is_client)
			cactivator = other.enemy;
		else if (other.oldenemy.has_value() && other.oldenemy->is_client)
			cactivator = other.oldenemy;
		else if (other.activator.has_value() && other.activator->is_client)
			cactivator = other.activator;
		else
			cactivator = other;
		G_UseTargets(self, cactivator);
		self.target = savetarget;
	}
}

REGISTER_STATIC_SAVABLE(point_combat_touch);

static void SP_point_combat(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}
	self.solid = SOLID_TRIGGER;
	self.touch = SAVABLE(point_combat_touch);
	self.bounds = {
		.mins = { -8, -8, -16 },
		.maxs = { 8, 8, 16 }
	};
	self.svflags = SVF_NOCLIENT;
	gi.linkentity(self);
}

REGISTER_ENTITY(POINT_COMBAT, point_combat);
#endif

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/

static REGISTER_ENTITY_NOSPAWN(INFO_NULL, info_null);

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
static void SP_info_notnull(entity &self)
{
	self.absbounds.mins = self.absbounds.maxs = self.origin;
}

static REGISTER_ENTITY(INFO_NOTNULL, info_notnull);

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/
#ifdef SINGLE_PLAYER

const spawn_flag START_OFF	= (spawn_flag)1;

static void light_use(entity &self, entity &, entity &)
{
	if (self.spawnflags & START_OFF) {
		gi.configstring((config_string)(CS_LIGHTS + self.style), "m");
		self.spawnflags &= ~START_OFF;
	} else {
		gi.configstring((config_string)(CS_LIGHTS + self.style), "a");
		self.spawnflags |= START_OFF;
	}
}

REGISTER_STATIC_SAVABLE(light_use);
#endif

static void SP_light(entity &self)
{
#ifdef SINGLE_PLAYER
	// no targeted lights in deathmatch, because they cause global messages
	if (!self.targetname || deathmatch)
	{
#endif
		G_FreeEdict(self);
#ifdef SINGLE_PLAYER
		return;
	}

	if (self.style >= 32)
	{
		self.use = SAVABLE(light_use);
		if (self.spawnflags & START_OFF)
			gi.configstring((config_string)(CS_LIGHTS + self.style), "a");
		else
			gi.configstring((config_string)(CS_LIGHTS + self.style), "m");
	}
#endif
}

REGISTER_ENTITY(LIGHT, light);

/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN   the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE          only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON        only valid for TRIGGER_SPAWN walls
				the wall will initially be present
*/

constexpr spawn_flag WALL_TRIGGER_SPAWN = (spawn_flag)1;
constexpr spawn_flag WALL_TOGGLE = (spawn_flag)2;
constexpr spawn_flag WALL_START_ON = (spawn_flag)4;
constexpr spawn_flag WALL_ANIMATED = (spawn_flag)8;
constexpr spawn_flag WALL_ANIMATED_FAST = (spawn_flag)16;

static void func_wall_use(entity &self, entity &, entity &)
{
	if (self.solid == SOLID_NOT)
	{
		self.solid = SOLID_BSP;
		self.svflags &= ~SVF_NOCLIENT;
		KillBox(self);
	}
	else
	{
		self.solid = SOLID_NOT;
		self.svflags |= SVF_NOCLIENT;
	}

	gi.linkentity(self);

	if (!(self.spawnflags & WALL_TOGGLE))
		self.use = nullptr;
}

REGISTER_STATIC_SAVABLE(func_wall_use);

static void SP_func_wall(entity &self)
{
	self.movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self.model);

	if (self.spawnflags & WALL_ANIMATED)
		self.effects |= EF_ANIM_ALL;
	if (self.spawnflags & WALL_ANIMATED_FAST)
		self.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if ((self.spawnflags & (WALL_START_ON | WALL_TOGGLE | WALL_TRIGGER_SPAWN)) == NO_SPAWNFLAGS)
	{
		self.solid = SOLID_BSP;
		gi.linkentity(self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self.spawnflags & WALL_TRIGGER_SPAWN))
		self.spawnflags |= WALL_TRIGGER_SPAWN;

	// yell if the spawnflags are odd
	if (self.spawnflags & WALL_START_ON)
	{
		if (!(self.spawnflags & WALL_TOGGLE))
		{
			gi.dprintfmt("{}: START_ON without TOGGLE\n", self);
			self.spawnflags |= WALL_TOGGLE;
		}
	}

	self.use = SAVABLE(func_wall_use);
	if (self.spawnflags & WALL_START_ON)
		self.solid = SOLID_BSP;
	else
	{
		self.solid = SOLID_NOT;
		self.svflags |= SVF_NOCLIENT;
	}
	gi.linkentity(self);
}

static REGISTER_ENTITY(FUNC_WALL, func_wall);

/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

constexpr spawn_flag OBJECT_ANIMATED = (spawn_flag)2;
constexpr spawn_flag OBJECT_ANIMATED_FAST = (spawn_flag)4;

static void func_object_touch(entity &self, entity &other, vector normal, const surface &)
{
	// only squash thing we fall on top of
	if (!normal)
		return;
	if (normal[2] < 1.0f)
		return;
	if (other.takedamage == false)
		return;
	T_Damage(other, self, self, vec3_origin, self.origin, vec3_origin, self.dmg, 1, { DAMAGE_NONE }, MOD_CRUSH);
}

REGISTER_STATIC_SAVABLE(func_object_touch);

static void func_object_release(entity &self)
{
	self.movetype = MOVETYPE_TOSS;
	self.touch = SAVABLE(func_object_touch);
}

REGISTER_STATIC_SAVABLE(func_object_release);

static void func_object_use(entity &self, entity &, entity &)
{
	self.solid = SOLID_BSP;
	self.svflags &= ~SVF_NOCLIENT;
	self.use = nullptr;
	KillBox(self);
	func_object_release(self);
}

REGISTER_STATIC_SAVABLE(func_object_use);

static void SP_func_object(entity &self)
{
	gi.setmodel(self, self.model);

	self.bounds.mins.x += 1;
	self.bounds.mins.y += 1;
	self.bounds.mins.z += 1;
	self.bounds.maxs.x -= 1;
	self.bounds.maxs.y -= 1;
	self.bounds.maxs.z -= 1;

	if (!self.dmg)
		self.dmg = 100;

	if (self.spawnflags == 0)
	{
		self.solid = SOLID_BSP;
		self.movetype = MOVETYPE_PUSH;
		self.think = SAVABLE(func_object_release);
		self.nextthink = level.time + 2_hz;
	}
	else
	{
		self.solid = SOLID_NOT;
		self.movetype = MOVETYPE_PUSH;
		self.use = SAVABLE(func_object_use);
		self.svflags |= SVF_NOCLIENT;
	}

	if (self.spawnflags & OBJECT_ANIMATED)
		self.effects |= EF_ANIM_ALL;
	if (self.spawnflags & OBJECT_ANIMATED_FAST)
		self.effects |= EF_ANIM_ALLFAST;

	self.clipmask = MASK_MONSTERSOLID;

	gi.linkentity(self);
}

static REGISTER_ENTITY(FUNC_OBJECT, func_object);
#ifdef SINGLE_PLAYER

/*QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST
Any brush that you want to explode or break apart.  If you want an
ex0plosion, set dmg and it will do a radius explosion of that amount
at the center of the bursh.

If targeted it will not be shootable.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/
static void func_explosive_explode(entity &self, entity &inflictor, entity &attacker, int32_t, vector)
{
	vector	origin;
	vector	chunkorigin;
	vector	csize;
	int	count;
	int	cmass;

	// bmodel origins are (0 0 0), we need to adjust that here
	csize = self.size * 0.5f;
	origin = self.absbounds.mins + csize;
	self.origin = origin;

	self.takedamage = false;

	if (self.dmg)
		T_RadiusDamage(self, attacker, self.dmg, 0, self.dmg + 40, MOD_EXPLOSIVE);

	self.velocity = self.origin - inflictor.origin;
	VectorNormalize(self.velocity);
	self.velocity *= 150;

	// start chunks towards the center
	csize *= 0.5f;

	cmass = self.mass;
	if (!cmass)
		cmass = 75;

	// big chunks
	if (cmass >= 100) {
		count = cmass / 100;
		if (count > 8)
			count = 8;
		while (count--) {
			chunkorigin = origin + crandomv(csize);
			ThrowDebris(self, "models/objects/debris1/tris.md2", 1, chunkorigin);
		}
	}

	// small chunks
	count = cmass / 25;
	if (count > 16)
		count = 16;
	while (count--) {
		chunkorigin = origin + crandomv(csize);
		ThrowDebris(self, "models/objects/debris2/tris.md2", 2, chunkorigin);
	}

#ifdef GROUND_ZERO
	// PMM - if we're part of a train, clean ourselves out of it
	if ((self.flags & FL_TEAMSLAVE) && self.teammaster.has_value() && self.teammaster->inuse)
	{
		for (entity &master : G_IterateChain<&entity::teamchain>(self.teammaster))
		{
			if (master.teamchain == self)
			{
				master.teamchain = self.teamchain;
				break;
			}
		}
	}
#endif

	G_UseTargets(self, attacker);

	if (self.dmg)
		BecomeExplosion1(self);
	else
		G_FreeEdict(self);
}

REGISTER_STATIC_SAVABLE(func_explosive_explode);

static void func_explosive_use(entity &self, entity &other, entity &)
{
	func_explosive_explode(self, self, other, self.health, vec3_origin);
}

REGISTER_STATIC_SAVABLE(func_explosive_use);

#ifdef GROUND_ZERO
static void func_explosive_activate(entity &self, entity &other, entity &cactivator)
{
	bool approved = false;

	if (other.target)
	{
		if (other.target == self.targetname)
			approved = true;
	}
	if (!approved && cactivator.target)
	{
		if (cactivator.target == self.targetname)
			approved = true;
	}

	if (!approved)
		return;

	self.use = SAVABLE(func_explosive_use);
	if (!self.health)
		self.health = 100;
	self.die = SAVABLE(func_explosive_explode);
	self.takedamage = true;
}

REGISTER_STATIC_SAVABLE(func_explosive_activate);
#endif

static void func_explosive_spawn(entity &self, entity &, entity &)
{
	self.solid = SOLID_BSP;
	self.svflags &= ~SVF_NOCLIENT;
	self.use = nullptr;
	KillBox(self);
	gi.linkentity(self);
}

REGISTER_STATIC_SAVABLE(func_explosive_spawn);

static constexpr spawn_flag EXPLOSIVE_TRIGGERED = (spawn_flag) 1;
static constexpr spawn_flag EXPLOSIVE_ANIMATED = (spawn_flag) 2;
static constexpr spawn_flag EXPLOSIVE_ANIMATED_FAST = (spawn_flag) 4;
#ifdef GROUND_ZERO
static constexpr spawn_flag EXPLOSIVE_ACTIVATED = (spawn_flag) 8;
#endif

static void SP_func_explosive(entity &self)
{
	if (deathmatch)
	{
		// auto-remove for deathmatch
		G_FreeEdict(self);
		return;
	}

	self.movetype = MOVETYPE_PUSH;

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");

	gi.setmodel(self, self.model);

	if (self.spawnflags & EXPLOSIVE_TRIGGERED)
	{
		self.svflags |= SVF_NOCLIENT;
		self.solid = SOLID_NOT;
		self.use = SAVABLE(func_explosive_spawn);
	}
#ifdef GROUND_ZERO
	else if (self.spawnflags & EXPLOSIVE_ACTIVATED)
	{
		self.solid = SOLID_BSP;

		if(self.targetname)
			self.use = SAVABLE(func_explosive_activate);
	}
#endif
	else
	{
		self.solid = SOLID_BSP;

		if (self.targetname)
			self.use = SAVABLE(func_explosive_use);
	}

	if (self.spawnflags & EXPLOSIVE_ANIMATED)
		self.effects |= EF_ANIM_ALL;
	if (self.spawnflags & EXPLOSIVE_ANIMATED_FAST)
		self.effects |= EF_ANIM_ALLFAST;

	if (self.use != SAVABLE(func_explosive_use)
#ifdef GROUND_ZERO
		&& self.use != func_explosive_activate
#endif
		) {
		if (!self.health)
			self.health = 100;
		self.die = SAVABLE(func_explosive_explode);
		self.takedamage = true;
	}

	gi.linkentity(self);
}

static REGISTER_ENTITY(FUNC_EXPLOSIVE, func_explosive);

#include "move.h"

/*QUAKED misc_explobox (0 .5 .8) (-16 -16 0) (16 16 40)
Large exploding box.  You can override its mass (100),
health (80), and dmg (150).
*/
static void barrel_touch(entity &self, entity &other, vector, const surface &)
{
	float	ratio;
	vector	v;

	if (!other.groundentity.has_value() || (other.groundentity == self))
		return;

	ratio = (float) other.mass / (float) self.mass;
	v = self.origin - other.origin;
	M_walkmove(self, vectoyaw(v), 20 * ratio * FRAMETIME.count());
}

REGISTER_STATIC_SAVABLE(barrel_touch);

static void barrel_explode(entity &self)
{
	vector	org;
	float   spd;
	vector	save;

	T_RadiusDamage(self, self.activator, self.dmg, 0, self.dmg + 40, MOD_EXPLOSIVE);

	save = self.origin;
	self.origin = self.absbounds.mins + (0.5f * self.size);

	// a few big chunks
	spd = 1.5f * (float)self.dmg / 200.0f;
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);

	// bottom corners
	spd = 1.75f * (float)self.dmg / 200.0f;
	org = self.absbounds.mins;
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	org = self.absbounds.mins;
	org.x += self.size.x;
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	org = self.absbounds.mins;
	org.y += self.size.y;
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	org = self.absbounds.mins;
	org.x += self.size.x;
	org.y += self.size.y;
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);

	// a bunch of little chunks
	spd = 2.f * self.dmg / 200.f;
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org = self.origin + crandomv(self.size);
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);

	self.origin = save;
	if (self.groundentity != null_entity)
		BecomeExplosion2(self);
	else
		BecomeExplosion1(self);
}

REGISTER_STATIC_SAVABLE(barrel_explode);

static void barrel_delay(entity &self, entity &, entity &attacker, int, vector)
{
	self.takedamage = false;
	self.nextthink = level.time + 20ms;
	self.think = SAVABLE(barrel_explode);
	self.activator = attacker;
}

REGISTER_STATIC_SAVABLE(barrel_delay);

#ifdef GROUND_ZERO
static void barrel_think(entity &self);

REGISTER_STATIC_SAVABLE(barrel_think);

static void barrel_think(entity &self)
{
	// the think needs to be first since later stuff may override.
	self.think = SAVABLE(barrel_think);
	self.nextthink = level.time + 1_hz;

	M_CatagorizePosition (self);
	self.flags |= FL_IMMUNE_SLIME;
	self.air_finished_time = level.time + 100s;
	M_WorldEffects (self);
}

static void barrel_start(entity &self)
{
	M_droptofloor(self);
	self.think = SAVABLE(barrel_think);
	self.nextthink = level.time + 1_hz;
}

REGISTER_STATIC_SAVABLE(barrel_start);
#endif

#include "monster.h"

static void SP_misc_explobox(entity &self)
{
	if (deathmatch)
	{
		// auto-remove for deathmatch
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.modelindex("models/objects/debris3/tris.md2");

	self.solid = SOLID_BBOX;
	self.movetype = MOVETYPE_STEP;

	self.model = "models/objects/barrels/tris.md2";
	self.modelindex = gi.modelindex(self.model);
	self.bounds = {
		.mins = { -16, -16, 0 },
		.maxs = { 16, 16, 40 }
	};

	if (!self.mass)
		self.mass = 400;
	if (!self.health)
		self.health = 10;
	if (!self.dmg)
		self.dmg = 150;

	self.die = SAVABLE(barrel_delay);
	self.takedamage = true;
	self.monsterinfo.aiflags = AI_NOSTEP;

	self.touch = SAVABLE(barrel_touch);

#ifdef GROUND_ZERO
	self.think = SAVABLE(barrel_start);
#else
	self.think = SAVABLE(M_droptofloor);
#endif
	self.nextthink = level.time + 2_hz;

	gi.linkentity(self);
}

REGISTER_ENTITY(MISC_EXPLOBOX, misc_explobox);
#endif

//
// miscellaneous specialty items
//

/*QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8)
*/

static void misc_blackhole_use(entity &ent, entity &, entity &)
{
	G_FreeEdict(ent);
}

REGISTER_STATIC_SAVABLE(misc_blackhole_use);

static void misc_blackhole_think(entity &self)
{
	if (++self.frame < 19)
		self.nextthink = level.time + 100ms;
	else
	{
		self.frame = 0;
		self.nextthink = level.time + 100ms;
	}
}

REGISTER_STATIC_SAVABLE(misc_blackhole_think);

static void SP_misc_blackhole(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_NOT;
	ent.bounds = {
		.mins = { -64, -64, 0 },
		.maxs = { 64, 64, 8 }
	};
	ent.modelindex = gi.modelindex("models/objects/black/tris.md2");
	ent.renderfx = RF_TRANSLUCENT;
	ent.use = SAVABLE(misc_blackhole_use);
	ent.think = SAVABLE(misc_blackhole_think);
	ent.nextthink = level.time + 2_hz;
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_BLACKHOLE, misc_blackhole);

/*QUAKED misc_eastertank (1 .5 0) (-32 -32 -16) (32 32 32)
*/

static void misc_eastertank_think(entity &self)
{
	if (++self.frame < 293)
		self.nextthink = level.time + 100ms;
	else
	{
		self.frame = 254;
		self.nextthink = level.time + 100ms;
	}
}

REGISTER_STATIC_SAVABLE(misc_eastertank_think);

static void SP_misc_eastertank(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.bounds = {
		.mins = { -32, -32, -16 },
		.maxs = { 32, 32, 32 }
	};
	ent.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	ent.frame = 254;
	ent.think = SAVABLE(misc_eastertank_think);
	ent.nextthink = level.time + 2_hz;
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_EASTERTANK, misc_eastertank);

/*QUAKED misc_easterchick (1 .5 0) (-32 -32 0) (32 32 32)
*/
static void misc_easterchick_think(entity &self)
{
	if (++self.frame < 247)
		self.nextthink = level.time + 100ms;
	else
	{
		self.frame = 208;
		self.nextthink = level.time + 100ms;
	}
}

REGISTER_STATIC_SAVABLE(misc_easterchick_think);

static void SP_misc_easterchick(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.bounds = {
		.mins = { -32, -32, 0 },
		.maxs = { 32, 32, 32 }
	};
	ent.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent.frame = 208;
	ent.think = SAVABLE(misc_easterchick_think);
	ent.nextthink = level.time + 2_hz;
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_EASTERCHICK, misc_easterchick);

/*QUAKED misc_easterchick2 (1 .5 0) (-32 -32 0) (32 32 32)
*/
static void misc_easterchick2_think(entity &self)
{
	if (++self.frame < 287)
		self.nextthink = level.time + 100ms;
	else
	{
		self.frame = 248;
		self.nextthink = level.time + 100ms;
	}
}

REGISTER_STATIC_SAVABLE(misc_easterchick2_think);

static void SP_misc_easterchick2(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.bounds = {
		.mins = { -32, -32, 0 },
		.maxs = { 32, 32, 32 }
	};
	ent.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent.frame = 248;
	ent.think = SAVABLE(misc_easterchick2_think);
	ent.nextthink = level.time + 2_hz;
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_EASTERCHICK2, misc_easterchick2);

/*QUAKED monster_commander_body (1 .5 0) (-32 -32 0) (32 32 48)
Not really a monster, this is the Tank Commander's decapitated body.
There should be a item_commander_head that has this as it's target.
*/
static void commander_body_think(entity &self)
{
	if (++self.frame < 24)
		self.nextthink = level.time + 100ms;
	else
		self.nextthink = gtime::zero();

	if (self.frame == 22)
		gi.sound(self, CHAN_BODY, gi.soundindex("tank/thud.wav"));
}

REGISTER_STATIC_SAVABLE(commander_body_think);

static void commander_body_use(entity &self, entity &, entity &)
{
	self.think = SAVABLE(commander_body_think);
	self.nextthink = level.time + 1_hz;
	gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"));
}

REGISTER_STATIC_SAVABLE(commander_body_use);

static void commander_body_drop(entity &self)
{
	self.movetype = MOVETYPE_TOSS;
	self.origin[2] += 2;
}

REGISTER_STATIC_SAVABLE(commander_body_drop);

static void SP_monster_commander_body(entity &self)
{
	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_BBOX;
	self.model = "models/monsters/commandr/tris.md2";
	self.modelindex = gi.modelindex(self.model);
	self.bounds = {
		.mins = { -32, -32, 0 },
		.maxs = { 32, 32, 48 }
	};
	self.use = SAVABLE(commander_body_use);
	self.takedamage = true;
	self.flags = FL_GODMODE;
	self.renderfx |= RF_FRAMELERP;
	gi.linkentity(self);

	gi.soundindex("tank/thud.wav");
	gi.soundindex("tank/pain.wav");

	self.think = SAVABLE(commander_body_drop);
	self.nextthink = level.time + 5_hz;
}

static REGISTER_ENTITY(MONSTER_COMMANDER_BODY, monster_commander_body);

/*QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4)
The origin is the bottom of the banner.
The banner is 128 tall.
*/
static void misc_banner_think(entity &ent)
{
	ent.frame = (ent.frame + 1) % 16;
	ent.nextthink = level.time + 100ms;
}

REGISTER_STATIC_SAVABLE(misc_banner_think);

static void SP_misc_banner(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_NOT;
	ent.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	ent.frame = Q_rand() % 16;
	gi.linkentity(ent);

	ent.think = SAVABLE(misc_banner_think);
	ent.nextthink = level.time + 1_hz;
}

static REGISTER_ENTITY(MISC_BANNER, misc_banner);
#ifdef SINGLE_PLAYER

/*QUAKED misc_deadsoldier (1 .5 0) (-16 -16 0) (16 16 16) ON_BACK ON_STOMACH BACK_DECAP FETAL_POS SIT_DECAP IMPALED
This is the dead player model. Comes in 6 exciting different poses!
*/
static void misc_deadsoldier_die(entity &self, entity &, entity &, int32_t damage, vector)
{
	int	n;

#ifdef GROUND_ZERO
	if (self.health > -30)
#else
	if (self.health > -80)
#endif
		return;

	gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"));
	for (n = 0; n < 4; n++)
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
}

REGISTER_STATIC_SAVABLE(misc_deadsoldier_die);

static void SP_misc_deadsoldier(entity &ent)
{
	if (deathmatch)
	{
		// auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.modelindex = gi.modelindex("models/deadbods/dude/tris.md2");

	// Defaults to frame 0
	if (ent.spawnflags & 2)
		ent.frame = 1;
	else if (ent.spawnflags & 4)
		ent.frame = 2;
	else if (ent.spawnflags & 8)
		ent.frame = 3;
	else if (ent.spawnflags & 16)
		ent.frame = 4;
	else if (ent.spawnflags & 32)
		ent.frame = 5;
	else
		ent.frame = 0;

	ent.bounds = {
		.mins = { -16, -16, 0 },
		.maxs = { 16, 16, 16 }
	};
	ent.deadflag = true;
	ent.takedamage = true;
	ent.svflags |= SVF_MONSTER | SVF_DEADMONSTER;
	ent.die = SAVABLE(misc_deadsoldier_die);
	ent.monsterinfo.aiflags |= AI_GOOD_GUY;

	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_DEADSOLDIER, misc_deadsoldier);
#endif
/*QUAKED misc_viper (1 .5 0) (-16 -16 0) (16 16 32)
This is the Viper for the flyby bombing.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"     How fast the Viper should fly
*/

#include "func.h"

void misc_viper_use(entity &self, entity &other, entity &cactivator)
{
	self.svflags &= ~SVF_NOCLIENT;
	self.use = SAVABLE(train_use);
	train_use(self, other, cactivator);
}

REGISTER_SAVABLE(misc_viper_use);

static void SP_misc_viper(entity &ent)
{
	if (!ent.target)
	{
		gi.dprintfmt("{}: no target\n", ent);
		G_FreeEdict(ent);
		return;
	}

	if (!ent.speed)
		ent.speed = 300.f;

	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_NOT;
	ent.modelindex = gi.modelindex("models/ships/viper/tris.md2");
	ent.bounds = {
		.mins = { -16, -16, 0 },
		.maxs = { 16, 16, 32 }
	};

	ent.think = SAVABLE(func_train_find);
	ent.nextthink = level.time + 1_hz;
	ent.use = SAVABLE(misc_viper_use);
	ent.svflags |= SVF_NOCLIENT;
	ent.moveinfo.accel = ent.moveinfo.decel = ent.moveinfo.speed = ent.speed;

	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_VIPER, misc_viper);

/*QUAKED misc_bigviper (1 .5 0) (-176 -120 -24) (176 120 72)
This is a large stationary viper as seen in Paul's intro
*/
static void SP_misc_bigviper(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.bounds = {
		.mins = { -176, -120, -24 },
		.maxs = { 176, 120, 72 }
	};
	ent.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_BIGVIPER, misc_bigviper);

/*QUAKED misc_viper_bomb (1 0 0) (-8 -8 -8) (8 8 8)
"dmg"   how much boom should the bomb make?
*/
static void misc_viper_bomb_touch(entity &self, entity &, vector, const surface &)
{
	G_UseTargets(self, self.activator);

	self.origin[2] = self.absbounds.mins.z + 1;
	T_RadiusDamage(self, self, (float)self.dmg, 0, (float)(self.dmg + 40), MOD_EXPLOSIVE);
	BecomeExplosion2(self);
}

REGISTER_STATIC_SAVABLE(misc_viper_bomb_touch);

static void misc_viper_bomb_prethink(entity &self)
{
	self.groundentity = null_entity;

	float diff = duration_cast<gtimef>(self.timestamp - level.time).count();
	if (diff < -1.0f)
		diff = -1.0f;

	vector v = self.moveinfo.dir * (1.0f + diff);
	v.z = diff;

	diff = self.angles[2];
	self.angles = vectoangles(v);
	self.angles[2] = diff + 10;
}

REGISTER_STATIC_SAVABLE(misc_viper_bomb_prethink);

static void misc_viper_bomb_use(entity &self, entity &, entity &cactivator)
{
	self.solid = SOLID_BBOX;
	self.svflags &= ~SVF_NOCLIENT;
	self.effects |= EF_ROCKET;
	self.use = nullptr;
	self.movetype = MOVETYPE_TOSS;
	self.prethink = SAVABLE(misc_viper_bomb_prethink);
	self.touch = SAVABLE(misc_viper_bomb_touch);
	self.activator = cactivator;

	entityref viper = G_FindEquals<&entity::type>(world, ET_MISC_VIPER);
	self.velocity = viper->moveinfo.dir * viper->moveinfo.speed;

	self.timestamp = level.time;
	self.moveinfo.dir = viper->moveinfo.dir;
}

REGISTER_STATIC_SAVABLE(misc_viper_bomb_use);

static void SP_misc_viper_bomb(entity &self)
{
	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_NOT;
	self.bounds = bbox::sized(8.f);

	self.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	if (!self.dmg)
		self.dmg = 1000;

	self.use = SAVABLE(misc_viper_bomb_use);
	self.svflags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

static REGISTER_ENTITY(MISC_VIPER_BOMB, misc_viper_bomb);

/*QUAKED misc_strogg_ship (1 .5 0) (-16 -16 0) (16 16 32)
This is a Storgg ship for the flybys.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"     How fast it should fly
*/
void misc_strogg_ship_use(entity &self, entity &other, entity &cactivator)
{
	self.svflags &= ~SVF_NOCLIENT;
	self.use = SAVABLE(train_use);
	train_use(self, other, cactivator);
}

REGISTER_SAVABLE(misc_strogg_ship_use);

static void SP_misc_strogg_ship(entity &ent)
{
	if (!ent.target)
	{
		gi.dprintfmt("{}: no target\n", ent);
		G_FreeEdict(ent);
		return;
	}

	if (!ent.speed)
		ent.speed = 300.f;

	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_NOT;
	ent.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	ent.bounds = {
		.mins = { -16, -16, 0 },
		.maxs = { 16, 16, 32 }
	};

	ent.think = SAVABLE(func_train_find);
	ent.nextthink = level.time + 1_hz;
	ent.use = SAVABLE(misc_strogg_ship_use);
	ent.svflags |= SVF_NOCLIENT;
	ent.moveinfo.accel = ent.moveinfo.decel = ent.moveinfo.speed = ent.speed;

	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_STROGG_SHIP, misc_strogg_ship);

/*QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128)
*/
static void misc_satellite_dish_think(entity &self)
{
	self.frame++;
	if (self.frame < 38)
		self.nextthink = level.time + 100ms;
}

REGISTER_STATIC_SAVABLE(misc_satellite_dish_think);

static void misc_satellite_dish_use(entity &self, entity &, entity &)
{
	self.frame = 0;
	self.think = SAVABLE(misc_satellite_dish_think);
	self.nextthink = level.time + 1_hz;
}

REGISTER_STATIC_SAVABLE(misc_satellite_dish_use);

static void SP_misc_satellite_dish(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.bounds = {
		.mins = { -64, -64, 0 },
		.maxs = { 64, 64, 128 }
	};
	ent.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
	ent.use = SAVABLE(misc_satellite_dish_use);
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_SATELLITE_DISH, misc_satellite_dish);

/*QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12)
*/
static void SP_light_mine1(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");
	gi.linkentity(ent);
}

static REGISTER_ENTITY(LIGHT_MINE1, light_mine1);

/*QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12)
*/
static void SP_light_mine2(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.modelindex = gi.modelindex("models/objects/minelite/light2/tris.md2");
	gi.linkentity(ent);
}

static REGISTER_ENTITY(LIGHT_MINE2, light_mine2);

/*QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
static void SP_misc_gib_arm(entity &ent)
{
	gi.setmodel(ent, "models/objects/gibs/arm/tris.md2");
	ent.solid = SOLID_NOT;
	ent.effects |= EF_GIB;
	ent.takedamage = true;
	ent.die = SAVABLE(gib_die);
	ent.movetype = MOVETYPE_TOSS;
	ent.svflags |= SVF_MONSTER;
	ent.deadflag = true;
	ent.avelocity = randomv({ 200, 200, 200 });
	ent.think = SAVABLE(G_FreeEdict);
	ent.nextthink = level.time + 30s;
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_GIB_ARM, misc_gib_arm);

/*QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
static void SP_misc_gib_leg(entity &ent)
{
	gi.setmodel(ent, "models/objects/gibs/leg/tris.md2");
	ent.solid = SOLID_NOT;
	ent.effects |= EF_GIB;
	ent.takedamage = true;
	ent.die = SAVABLE(gib_die);
	ent.movetype = MOVETYPE_TOSS;
	ent.svflags |= SVF_MONSTER;
	ent.deadflag = true;
	ent.avelocity = randomv({ 200, 200, 200 });
	ent.think = SAVABLE(G_FreeEdict);
	ent.nextthink = level.time + 30s;
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_GIB_LEG, misc_gib_leg);

/*QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
static void SP_misc_gib_head(entity &ent)
{
	gi.setmodel(ent, "models/objects/gibs/head/tris.md2");
	ent.solid = SOLID_NOT;
	ent.effects |= EF_GIB;
	ent.takedamage = true;
	ent.die = SAVABLE(gib_die);
	ent.movetype = MOVETYPE_TOSS;
	ent.svflags |= SVF_MONSTER;
	ent.deadflag = true;
	ent.avelocity = randomv({ 200, 200, 200 });
	ent.think = SAVABLE(G_FreeEdict);
	ent.nextthink = level.time + 30s;
	gi.linkentity(ent);
}

static REGISTER_ENTITY(MISC_GIB_HEAD, misc_gib_head);

//=====================================================

/*QUAKED target_character (0 0 1) ?
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

static void SP_target_character(entity &self)
{
	self.movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self.model);
	self.solid = SOLID_BSP;
	self.frame = 12;
	gi.linkentity(self);
}

static REGISTER_ENTITY(TARGET_CHARACTER, target_character);

/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
*/
static void target_string_use(entity &self, entity &, entity &)
{
	size_t l = strlen(self.message);

	for (entity &e : G_IterateChain<&entity::teamchain>(self.teammaster))
	{
		if (!e.count)
			continue;
		int32_t n = e.count - 1;
		if (n > (int32_t) l)
		{
			e.frame = 12;
			continue;
		}

		char c = self.message[n];
		if (c >= '0' && c <= '9')
			e.frame = (int32_t)(c - '0');
		else if (c == '-')
			e.frame = 10;
		else if (c == ':')
			e.frame = 11;
		else
			e.frame = 12;
	}
}

REGISTER_STATIC_SAVABLE(target_string_use);

static void SP_target_string(entity &self)
{
	self.use = SAVABLE(target_string_use);
}

static REGISTER_ENTITY(TARGET_STRING, target_string);

/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and the fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"     0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

constexpr spawn_flag CLOCK_TIMER_UP = (spawn_flag)1;
constexpr spawn_flag CLOCK_TIMER_DOWN = (spawn_flag)2;
constexpr spawn_flag CLOCK_START_OFF = (spawn_flag)4;
constexpr spawn_flag CLOCK_MULTI_USE = (spawn_flag)8;

static void func_clock_reset(entity &self)
{
	self.activator = null_entity;

	if (self.spawnflags & CLOCK_TIMER_UP)
	{
		self.health = 0;
		self.volume = (float) self.count;
	}
	else if (self.spawnflags & CLOCK_TIMER_DOWN)
	{
		self.health = self.count;
		self.volume = 0;
	}
}

static void func_clock_format_countdown(entity &self)
{
	if (self.style == 0)
		self.message = format("{:2}", self.health);
	else if (self.style == 1)
		self.message = format("{:02}:{:02}", self.health / 60, self.health % 60);
	else if (self.style == 2)
		self.message = format("{:02}:{:02}:{:02}", self.health / 3600, (self.health - (self.health / 3600) * 3600) / 60, self.health % 60);
}

static void func_clock_think(entity &self)
{
	if (!self.enemy.has_value())
	{
		self.enemy = G_FindFunc<&entity::targetname>(world, self.target, striequals);
		if (!self.enemy.has_value())
			return;
	}

	if (self.spawnflags & CLOCK_TIMER_UP)
	{
		func_clock_format_countdown(self);
		self.health++;
	}
	else if (self.spawnflags & CLOCK_TIMER_DOWN)
	{
		func_clock_format_countdown(self);
		self.health--;
	}
	else
	{
		time_t now = _time64(nullptr);
		tm ltime;
		_localtime64_s(&ltime, &now);
		self.message = format("{:02}:{:02}:{:02}", ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	}

	self.enemy->message = self.message;
	self.enemy->use(self.enemy, self, self);

	if (((self.spawnflags & CLOCK_TIMER_UP) && (self.health > self.volume)) ||
		((self.spawnflags & CLOCK_TIMER_DOWN) && (self.health < self.volume)))
	{
		if (self.pathtarget)
		{
			string	savetarget = self.target;
			string	savemessage = self.message;
			self.target = self.pathtarget;
			self.message = nullptr;
			G_UseTargets(self, self.activator);
			self.target = savetarget;
			self.message = savemessage;
		}

		if (!(self.spawnflags & CLOCK_MULTI_USE))
			return;

		func_clock_reset(self);

		if (self.spawnflags & CLOCK_START_OFF)
			return;
	}

	self.nextthink = level.time + 1s;
}

REGISTER_STATIC_SAVABLE(func_clock_think);

static void func_clock_use(entity &self, entity &, entity &cactivator)
{
	if (!(self.spawnflags & CLOCK_MULTI_USE))
		self.use = nullptr;
	if (self.activator.has_value())
		return;
	self.activator = cactivator;
	self.think(self);
}

REGISTER_STATIC_SAVABLE(func_clock_use);

static void SP_func_clock(entity &self)
{
	if (!self.target)
	{
		gi.dprintfmt("{}: no target\n", self);
		G_FreeEdict(self);
		return;
	}

	if ((self.spawnflags & CLOCK_TIMER_DOWN) && (!self.count))
	{
		gi.dprintfmt("{}: no count\n", self);
		G_FreeEdict(self);
		return;
	}

	if ((self.spawnflags & CLOCK_TIMER_UP) && (!self.count))
		self.count = 60 * 60;

	func_clock_reset(self);

	self.think = SAVABLE(func_clock_think);

	if (self.spawnflags & CLOCK_START_OFF)
		self.use = SAVABLE(func_clock_use);
	else
		self.nextthink = level.time + 1s;
}

static REGISTER_ENTITY(FUNC_CLOCK, func_clock);

//=================================================================================

#ifdef HOOK_CODE
#include "game/ctf/grapple.h"
#endif

static void teleporter_touch(entity &self, entity &other, vector, const surface &)
{
	if (!other.is_client)
		return;

	entityref dest = G_FindFunc<&entity::targetname>(world, self.target, striequals);

	if (!dest.has_value())
	{
		gi.dprintfmt("{}: couldn't find destination\n", self);
		return;
	}

#ifdef HOOK_CODE
	GrapplePlayerReset(other);
#endif

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity(other);

	other.origin = other.old_origin = dest->origin;
	other.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	other.velocity = vec3_origin;
	other.client.ps.pmove.pm_time = 160 >> 3;     // hold time
	other.client.ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// draw the teleport splash at source and on the player
	self.owner->event = EV_PLAYER_TELEPORT;
	other.event = EV_PLAYER_TELEPORT;

	// set angles
	other.client.ps.pmove.set_delta_angles(dest->angles - other.client.resp.cmd_angles);

	other.angles = vec3_origin;
	other.client.ps.viewangles = vec3_origin;
	other.client.v_angle = vec3_origin;

	// kill anything at the destination
	KillBox(other);

	gi.linkentity(other);
}

REGISTER_STATIC_SAVABLE(teleporter_touch);

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16)
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
static void SP_misc_teleporter(entity &ent)
{
	if (!ent.target)
	{
		gi.dprintfmt("{}: no target.\n", ent);
		G_FreeEdict(ent);
		return;
	}

	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent.skinnum = 1;
	ent.effects = EF_TELEPORTER;
	ent.sound = gi.soundindex("world/amb10.wav");
	ent.solid = SOLID_BBOX;

	ent.bounds = {
		.mins = { -32, -32, -24 },
		.maxs = { 32, 32, -16 }
	};
	gi.linkentity(ent);

	entity &trig = G_Spawn();
	trig.touch = SAVABLE(teleporter_touch);
	trig.solid = SOLID_TRIGGER;
	trig.target = ent.target;
	trig.owner = ent;
	trig.origin = ent.origin;
	trig.bounds = {
		.mins = { -8, -8, 8 },
		.maxs = { 8, 8, 24 }
	};
	gi.linkentity(trig);
}

static REGISTER_ENTITY(MISC_TELEPORTER, misc_teleporter);

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
*/
void SP_misc_teleporter_dest(entity &ent)
{
    gi.setmodel(ent, "models/objects/dmspot/tris.md2");
    ent.skinnum = 0;
    ent.solid = SOLID_BBOX;
	ent.bounds = {
		.mins = { -32, -32, -24 },
		.maxs = { 32, 32, -16 }
	};
    gi.linkentity(ent);
}

REGISTER_ENTITY(MISC_TELEPORTER_DEST, misc_teleporter_dest);
