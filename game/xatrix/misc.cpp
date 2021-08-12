#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/misc.h"
#include "game/spawn.h"
#include "game/game.h"
#include "lib/string/format.h"
#include "misc.h"

void ThrowGibACID(entity &self, stringlit gibname, int32_t damage, gib_type type)
{
	entity &gib = ThrowGib(self, gibname, damage, type);

	gib.effects &= ~EF_GIB;
	gib.effects |= EF_GREENGIB;
	gib.renderfx |= RF_FULLBRIGHT;

	if (type == GIB_ORGANIC)
	{
		gib.velocity = self.velocity + (3.f * VelocityForDamage(damage));
		ClipGibVelocity(gib);
	}

	gi.linkentity (gib);
}

void ThrowHeadACID(entity &self, stringlit gibname, int32_t damage, gib_type type)
{
	ThrowHead(self, gibname, damage, type);
  
	self.effects &= ~EF_GIB;
	self.effects |= EF_GREENGIB;
	self.renderfx |= RF_FULLBRIGHT;
	self.sound = SOUND_NONE;
}

#include "game/func.h"

/*QUAKED misc_crashviper (1 .5 0) (-176 -120 -24) (176 120 72) 
This is a large viper about to crash
*/
static void SP_misc_crashviper(entity &ent)
{
	if (!ent.target)
	{
		gi.dprintfmt("{}: no target\n", ent);
		G_FreeEdict (ent);
		return;
	}

	if (!ent.speed)
		ent.speed = 300.f;

	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_NOT;
	ent.modelindex = gi.modelindex ("models/ships/bigviper/tris.md2");
	ent.bounds = {
		.mins = { -16, -16, 0 },
		.maxs = { 16, 16, 32 }
	};

	ent.think = SAVABLE(func_train_find);
	ent.nextthink = level.framenum + 1;
	ent.use = SAVABLE(misc_viper_use);
	ent.svflags |= SVF_NOCLIENT;
	ent.moveinfo.accel = ent.moveinfo.decel = ent.moveinfo.speed = ent.speed;

	gi.linkentity (ent);
}

static REGISTER_ENTITY(MISC_CRASHVIPER, misc_crashviper);

#ifdef SINGLE_PLAYER
#include "game/combat.h"
#include "game/util.h"

// RAFAEL
/*QUAKED misc_viper_missile (1 0 0) (-8 -8 -8) (8 8 8)
"dmg"	how much boom should the bomb make? the default value is 250
*/
static void misc_viper_missile_use(entity &self, entity &, entity &)
{
	self.enemy = G_FindEquals<&entity::targetname>(world, self.target);
	
	vector vec = self.enemy->origin;
	vec[2] += 16;	// Knightmare fixed
	
	vector start = self.origin;
	vector dir = vec - start;
	VectorNormalize (dir);
	
	monster_fire_rocket (self, start, dir, self.dmg, 500, MZ2_CHICK_ROCKET_1);
	
	self.nextthink = level.framenum + 1;
	self.think = SAVABLE(G_FreeEdict);
}

REGISTER_STATIC_SAVABLE(misc_viper_missile_use);

static void SP_misc_viper_missile(entity &self)
{
	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_NOT;
	self.bounds = bbox::sized(8.f);

	if (!self.dmg)
		self.dmg = 250;

	self.modelindex = gi.modelindex ("models/objects/bomb/tris.md2");

	self.use = SAVABLE(misc_viper_missile_use);
	self.svflags |= SVF_NOCLIENT;

	gi.linkentity (self);
}

static REGISTER_ENTITY(MISC_VIPER_MISSILE, misc_viper_missile);

#endif

/*QUAKED misc_transport (1 0 0) (-8 -8 -8) (8 8 8) TRIGGER_SPAWN
Maxx's transport at end of game
*/
static void SP_misc_transport(entity &ent)
{
	if (!ent.target)
	{
		gi.dprintfmt("{}: no target\n", ent);
		G_FreeEdict (ent);
		return;
	}

	if (!ent.speed)
		ent.speed = 300.f;

	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_NOT;
	ent.modelindex = gi.modelindex ("models/objects/ship/tris.md2");

	ent.bounds = {
		.mins = { -16, -16, 0 },
		.maxs = { 16, 16, 32 }
	};

	ent.think = SAVABLE(func_train_find);
	ent.nextthink = level.framenum + 1;
	ent.use = SAVABLE(misc_strogg_ship_use);
	ent.svflags |= SVF_NOCLIENT;
	ent.moveinfo.accel = ent.moveinfo.decel = ent.moveinfo.speed = ent.speed;

	if (!(ent.spawnflags & TRAIN_START_ON))
		ent.spawnflags |= TRAIN_START_ON;
	
	gi.linkentity (ent);
}

static REGISTER_ENTITY(MISC_TRANSPORT, misc_transport);

/*QUAKED misc_amb4 (1 0 0) (-16 -16 -16) (16 16 16)
Mal's amb4 loop entity
*/
static sound_index amb4sound;

static void amb4_think(entity &ent)
{
	ent.nextthink = level.framenum + (gtime)(2.7 * BASE_FRAMETIME);
	gi.sound(ent, CHAN_VOICE, amb4sound, ATTN_NONE);
}

REGISTER_STATIC_SAVABLE(amb4_think);

static void SP_misc_amb4(entity &ent)
{
	ent.think = SAVABLE(amb4_think);
	ent.nextthink = level.framenum + 1 * BASE_FRAMETIME;
	amb4sound = gi.soundindex ("world/amb4.wav");
	gi.linkentity (ent);
}

static REGISTER_ENTITY(MISC_AMB4, misc_amb4);

constexpr means_of_death MOD_NUKED { .self_kill_fmt = "{0} was de-atomized by a nuke.\n" };

/*QUAKED misc_nuke (1 0 0) (-16 -16 -16) (16 16 16)
*/
static void use_nuke(entity &self, entity &, entity &)
{
	for (uint32_t fromn = 1; fromn < num_entities; fromn++)
	{
		entity &from = itoe(fromn);
		
		if (from == self)
			continue;

		if (from.is_client())
			T_Damage (from, self, self, vec3_origin, from.origin, vec3_origin, 100000, 1, { DAMAGE_NONE }, MOD_NUKED);
		else if (from.svflags & SVF_MONSTER)
			G_FreeEdict (from);
	}

	self.use = nullptr;
}

REGISTER_STATIC_SAVABLE(use_nuke);

static void SP_misc_nuke(entity &ent)
{
	ent.use = SAVABLE(use_nuke);
}

static REGISTER_ENTITY(MISC_NUKE, misc_nuke);

#endif