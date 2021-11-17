#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/misc.h"
#include "game/spawn.h"
#include "game/game.h"
#include "game/util.h"
#include "game/target.h"
#include "lib/string/format.h"

/*QUAKED target_mal_laser (1 0 0) (-4 -4 -4) (4 4 4) START_ON RED GREEN BLUE YELLOW ORANGE FAT
Mal's laser
*/
static void target_mal_laser_on(entity &self)
{
	if (!self.activator.has_value())
		self.activator = self;
	self.spawnflags |= LASER_BZZT | LASER_ON;
	self.svflags &= ~SVF_NOCLIENT;
	self.nextthink = duration_cast<gtime>(level.time + self.wait + self.delay);
}

static void target_mal_laser_off(entity &self)
{
	self.spawnflags &= ~LASER_ON;
	self.svflags |= SVF_NOCLIENT;
	self.nextthink = gtime::zero();
}

static void target_mal_laser_use(entity &self, entity &, entity &cactivator)
{
	self.activator = cactivator;
	if (self.spawnflags & LASER_ON)
		target_mal_laser_off (self);
	else
		target_mal_laser_on (self);
}

REGISTER_STATIC_SAVABLE(target_mal_laser_use);

static void mal_laser_think(entity &self)
{
	target_laser_think (self);
	self.nextthink = duration_cast<gtime>(level.time + self.wait + 0.1s);
	self.spawnflags |= LASER_BZZT;
}

REGISTER_STATIC_SAVABLE(mal_laser_think);

static void SP_target_mal_laser(entity &self)
{
	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_NOT;
	self.renderfx |= RF_BEAM|RF_TRANSLUCENT;
	self.modelindex = MODEL_WORLD;			// must be non-zero

	// set the beam diameter
	if (self.spawnflags & LASER_FAT)
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

	G_SetMovedir (self.angles, self.movedir);
	
	if (self.delay == gtimef::zero())
		self.delay = 0.1s;

	if (self.wait == gtimef::zero())
		self.wait = 0.1s;

	if (!self.dmg)
		self.dmg = 5;

	self.bounds = bbox::sized(8.f);
	
	self.nextthink = duration_cast<gtime>(level.time + self.delay);
	self.think = SAVABLE(mal_laser_think);

	self.use = SAVABLE(target_mal_laser_use);

	gi.linkentity (self);

	if (self.spawnflags & LASER_ON)
		target_mal_laser_on (self);
	else
		target_mal_laser_off (self);
}

static REGISTER_ENTITY(TARGET_LASER_MAL, target_mal_laser);

#endif