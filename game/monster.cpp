import "config.h";

#ifdef SINGLE_PLAYER
import "entity.h";
import "game.h";
import "move.h";
import "ai.h";

import "lib/gi.h";
import "game/util.h";
import "lib/math/random.h";
import "lib/string/format.h";
import "game/ballistics/bullet.h";
import "game/ballistics/blaster.h";
import "game/ballistics/grenade.h";
import "game/ballistics/rocket.h";
import "game/ballistics/rail.h";
import "game/ballistics/bfg.h";
import "combat.h";

//
// monster weapons
//

void monster_fire_bullet(entity &self, vector start, vector dir, int damage, int kick, int hspread, int vspread, monster_muzzleflash flashtype)
{
	fire_bullet(self, start, dir, damage, kick, hspread, vspread, MOD_UNKNOWN);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self.s.number);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_shotgun(entity &self, vector start, vector aimdir, int damage, int kick, int hspread, int vspread, int count, monster_muzzleflash flashtype)
{
	fire_shotgun(self, start, aimdir, damage, kick, hspread, vspread, count, MOD_UNKNOWN);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self.s.number);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_blaster(entity &self, vector start, vector dir, int damage, int speed, monster_muzzleflash flashtype, entity_effects effect)
{
	fire_blaster(self, start, dir, damage, speed, effect, MOD_UNKNOWN, false);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self.s.number);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_grenade(entity &self, vector start, vector aimdir, int damage, int speed, monster_muzzleflash flashtype)
{
	fire_grenade(self, start, aimdir, damage, speed, 2.5f, damage + 40);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self.s.number);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_rocket(entity &self, vector start, vector dir, int damage, int speed, monster_muzzleflash flashtype)
{
	fire_rocket(self, start, dir, damage, speed, damage + 20, damage);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self.s.number);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_railgun(entity &self, vector start, vector aimdir, int damage, int kick, monster_muzzleflash flashtype)
{
	fire_rail(self, start, aimdir, damage, kick);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self.s.number);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_bfg(entity &self, vector start, vector aimdir, int damage, int speed, float damage_radius, monster_muzzleflash flashtype)
{
	fire_bfg(self, start, aimdir, damage, speed, damage_radius);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self.s.number);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

//
// Monster utility functions
//

void M_FliesOff(entity &self)
{
	self.s.effects &= ~EF_FLIES;
	self.s.sound = SOUND_NONE;
}

REGISTER_SAVABLE_FUNCTION(M_FliesOff);

void M_FliesOn(entity &self)
{
	if (self.waterlevel)
		return;
	self.s.effects |= EF_FLIES;
	self.s.sound = gi.soundindex("infantry/inflies1.wav");
	self.think = M_FliesOff_savable;
	self.nextthink = level.framenum + 60 * BASE_FRAMERATE;
}

REGISTER_SAVABLE_FUNCTION(M_FliesOn);

void M_FlyCheck(entity &self)
{
	if (self.waterlevel)
		return;

	if (random() > 0.5f)
		return;

	self.think = M_FliesOn_savable;
	self.nextthink = level.framenum + (gtime)random(5.f * BASE_FRAMERATE, 15.f * BASE_FRAMERATE);
}

REGISTER_SAVABLE_FUNCTION(M_FlyCheck);

void M_CheckGround(entity &ent)
{
	vector	point;
	trace	tr;

	if (ent.flags & (FL_SWIM | FL_FLY))
		return;

#ifdef GROUND_ZERO
	if ((ent.velocity.z * ent.gravityVector.z) < -100)		// PGM
#else
	if (ent.velocity.z > 100)
#endif
	{
		ent.groundentity = null_entity;
		return;
	}

// if the hull point one-quarter unit down is solid the entity is on ground
	point.x = ent.s.origin[0];
	point.y = ent.s.origin[1];
#ifdef GROUND_ZERO
	point.z = ent.s.origin[2] + (0.25f * ent.gravityVector[2]);	//PGM
#else
	point.z = ent.s.origin[2] - 0.25f;
#endif

	tr = gi.trace(ent.s.origin, ent.mins, ent.maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
#ifdef GROUND_ZERO
	if ((ent.gravityVector[2] < 0 ? (tr.normal[2] < 0.7f) : (tr.normal[2] > -0.7f)) && !tr.startsolid)
#else
	if (tr.normal[2] < 0.7f && !tr.startsolid)
#endif
	{
		ent.groundentity = null_entity;
		return;
	}

	if (!tr.startsolid && !tr.allsolid) {
		ent.s.origin = tr.endpos;
		ent.groundentity = tr.ent;
		ent.groundentity_linkcount = tr.ent.linkcount;
		ent.velocity.z = 0;
	}
}

void M_CatagorizePosition(entity &ent)
{
//
// get waterlevel
//
	vector point = ent.s.origin;
	point.z += ent.mins.z + 1;
	content_flags cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
	{
		ent.waterlevel = 0;
		ent.watertype = CONTENTS_NONE;
		return;
	}

	ent.watertype = cont;
	ent.waterlevel = 1;
	point.z += 26;
	cont = gi.pointcontents(point);
	if (!(cont & MASK_WATER))
		return;

	ent.waterlevel = 2;
	point.z += 22;
	cont = gi.pointcontents(point);
	if (cont & MASK_WATER)
		ent.waterlevel = 3;
}

void M_WorldEffects(entity &ent)
{
	int32_t dmg;

	if (ent.health > 0) {
		if (!(ent.flags & FL_SWIM)) {
			if (ent.waterlevel < 3) {
				ent.air_finished_framenum = level.framenum + 12 * BASE_FRAMERATE;
			} else if (ent.air_finished_framenum < level.framenum) {
				// drown!
				if (ent.pain_debounce_framenum < level.framenum) {
					dmg = (int32_t)(2 + 2 * ((level.framenum - ent.air_finished_framenum) / BASE_FRAMERATE));
					if (dmg > 15)
						dmg = 15;
					T_Damage(ent, world, world, vec3_origin, ent.s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
				}
			}
		} else {
			if (ent.waterlevel > 0) {
				ent.air_finished_framenum = level.framenum + 9 * BASE_FRAMERATE;
			} else if (ent.air_finished_framenum < level.framenum) {
				// suffocate!
				if (ent.pain_debounce_framenum < level.framenum) {
					dmg = (int32_t)(2 + 2 * ((level.framenum - ent.air_finished_framenum) / BASE_FRAMERATE));
					if (dmg > 15)
						dmg = 15;
					T_Damage(ent, world, world, vec3_origin, ent.s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
				}
			}
		}
	}

	if (ent.waterlevel == 0) {
		if (ent.flags & FL_INWATER) {
			gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent.flags &= ~FL_INWATER;
		}
		return;
	}

	if ((ent.watertype & CONTENTS_LAVA) && !(ent.flags & FL_IMMUNE_LAVA)) {
		if (ent.damage_debounce_framenum < level.framenum) {
			ent.damage_debounce_framenum = level.framenum + (int)(0.2f * BASE_FRAMERATE);
			T_Damage(ent, world, world, vec3_origin, ent.s.origin, vec3_origin, 10 * ent.waterlevel, 0, DAMAGE_NONE, MOD_LAVA);
		}
	}
	if ((ent.watertype & CONTENTS_SLIME) && !(ent.flags & FL_IMMUNE_SLIME)) {
		if (ent.damage_debounce_framenum < level.framenum) {
			ent.damage_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
			T_Damage(ent, world, world, vec3_origin, ent.s.origin, vec3_origin, 4 * ent.waterlevel, 0, DAMAGE_NONE, MOD_SLIME);
		}
	}

	if (!(ent.flags & FL_INWATER)) {
		if (!(ent.svflags & SVF_DEADMONSTER)) {
			if (ent.watertype & CONTENTS_LAVA)
				if (random() <= 0.5f)
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
			else if (ent.watertype & CONTENTS_SLIME)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			else if (ent.watertype & CONTENTS_WATER)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}

		ent.flags |= FL_INWATER;
		ent.damage_debounce_framenum = 0;
	}
}

void M_droptofloor(entity &ent)
{
	vector	end;
	trace	tr;

#ifdef GROUND_ZERO
	float grav = 1.0f - ent.gravityVector[2];
	ent.s.origin[2] += 1 * grav;
	end = ent.s.origin;
	end.z -= 256 * grav;
#else
	ent.s.origin[2] += 1;
	end = ent.s.origin;
	end.z -= 256;
#endif

	tr = gi.trace(ent.s.origin, ent.mins, ent.maxs, end, ent, MASK_MONSTERSOLID);

	if (tr.fraction == 1 || tr.allsolid)
		return;

	ent.s.origin = tr.endpos;

	gi.linkentity(ent);
	M_CheckGround(ent);
	M_CatagorizePosition(ent);
}

REGISTER_SAVABLE_FUNCTION(M_droptofloor);

void M_SetEffects(entity &ent)
{
	ent.s.effects &= ~(EF_COLOR_SHELL | EF_POWERSCREEN);
	ent.s.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);

	if (ent.monsterinfo.aiflags & AI_RESURRECTING) {
		ent.s.effects |= EF_COLOR_SHELL;
		ent.s.renderfx |= RF_SHELL_RED;
	}

	if (ent.health <= 0)
		return;

	if (ent.powerarmor_framenum > level.framenum) {
		if (ent.monsterinfo.power_armor_type == ITEM_POWER_SCREEN) {
			ent.s.effects |= EF_POWERSCREEN;
		} else if (ent.monsterinfo.power_armor_type == ITEM_POWER_SHIELD) {
			ent.s.effects |= EF_COLOR_SHELL;
			ent.s.renderfx |= RF_SHELL_GREEN;
		}
	}

#ifdef GROUND_ZERO
	ent.s.effects &= ~(EF_DOUBLE | EF_QUAD | EF_PENT);
	ent.s.renderfx &= ~RF_SHELL_DOUBLE;

	if (ent.monsterinfo.quad_framenum > level.framenum)
	{
		int remaining = ent.monsterinfo.quad_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4) )
			ent.s.effects |= EF_QUAD;
	}
	else
		ent.s.effects &= ~EF_QUAD;

	if (ent.monsterinfo.double_framenum > level.framenum)
	{
		int remaining = ent.monsterinfo.double_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4) )
			ent.s.effects |= EF_DOUBLE;
	}
	else
		ent.s.effects &= ~EF_DOUBLE;

	if (ent.monsterinfo.invincible_framenum > level.framenum)
	{
		int remaining = ent.monsterinfo.invincible_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4) )
			ent.s.effects |= EF_PENT;
	}
	else
		ent.s.effects &= ~EF_PENT;
#endif
}

static void M_MoveFrame(entity &self)
{
	mmove_t *move = self.monsterinfo.currentmove;
	self.nextthink = level.framenum + 1;

	if ((self.monsterinfo.nextframe) && (self.monsterinfo.nextframe >= move->firstframe) && (self.monsterinfo.nextframe <= move->lastframe)) {
		self.s.frame = self.monsterinfo.nextframe;
		self.monsterinfo.nextframe = 0;
	} else {
		if (self.s.frame == move->lastframe) {
			if (move->endfunc) {
				move->endfunc(self);

				// regrab move, endfunc is very likely to change it
				move = self.monsterinfo.currentmove;
				
				// check for death
				if (self.svflags & SVF_DEADMONSTER)
					return;
			}
		}

		if (self.s.frame < move->firstframe || self.s.frame > move->lastframe) {
			self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self.s.frame = move->firstframe;
		} else {
			if (!(self.monsterinfo.aiflags & AI_HOLD_FRAME)) {
				self.s.frame++;
				if (self.s.frame > move->lastframe)
					self.s.frame = move->firstframe;
			}
		}
	}

	int index = self.s.frame - move->firstframe;
	
	mframe_t &frame = move->frame[index];
	
	if (frame.aifunc) {
		if (!(self.monsterinfo.aiflags & AI_HOLD_FRAME))
			frame.aifunc(self, frame.dist * self.monsterinfo.scale);
		else
			frame.aifunc(self, 0);
	}

	if (frame.thinkfunc)
		frame.thinkfunc(self);
}


void monster_think(entity &self)
{
	M_MoveFrame(self);
	if (self.linkcount != self.monsterinfo.linkcount) {
		self.monsterinfo.linkcount = self.linkcount;
		M_CheckGround(self);
	}
	M_CatagorizePosition(self);
	M_WorldEffects(self);
	M_SetEffects(self);
}

REGISTER_SAVABLE_FUNCTION(monster_think);

void monster_use(entity &self, entity &, entity &cactivator)
{
	if (self.enemy.has_value())
		return;
	if (self.health <= 0)
		return;
	if (cactivator.flags & FL_NOTARGET)
		return;
	if (!(cactivator.is_client()) && !(cactivator.monsterinfo.aiflags & AI_GOOD_GUY))
		return;
#ifdef GROUND_ZERO
	if (cactivator.flags & FL_DISGUISED)
		return;
#endif

// delay reaction so if the monster is teleported, its sound is still heard
	self.enemy = cactivator;
	FoundTarget(self);
}

REGISTER_SAVABLE_FUNCTION(monster_use);

// from monster.qc
static void monster_start_go(entity &self);

static void monster_triggered_spawn(entity &self)
{
	self.s.origin[2] += 1;
	KillBox(self);

	self.solid = SOLID_BBOX;
	self.movetype = MOVETYPE_STEP;
	self.svflags &= ~SVF_NOCLIENT;
	self.air_finished_framenum = level.framenum + 12 * BASE_FRAMERATE;
	gi.linkentity(self);

	monster_start_go(self);

#ifdef THE_RECKONING
	// RAFAEL
	if (self.classname == "monster_fixbot")
	{
		if ((self.spawnflags & 16) || (self.spawnflags & 8) || (self.spawnflags & 4))
		{
			self.enemy = world;
			return;
		}
	}
#endif

	if (self.enemy.has_value() && !(self.spawnflags & 1) && !(self.enemy->flags & FL_VISIBLE_MASK))
		FoundTarget(self);
	else
		self.enemy = world;
}

REGISTER_SAVABLE_FUNCTION(monster_triggered_spawn);

static void monster_triggered_spawn_use(entity &self, entity &, entity &cactivator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self.think = monster_triggered_spawn_savable;
	self.nextthink = level.framenum + 1;
	if (cactivator.is_client())
		self.enemy = cactivator;
	self.use = monster_use_savable;
}

REGISTER_SAVABLE_FUNCTION(monster_triggered_spawn_use);

static void monster_triggered_start(entity &self)
{
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	self.svflags |= SVF_NOCLIENT;
	self.nextthink = 0;
	self.use = monster_triggered_spawn_use_savable;
}


void monster_death_use(entity &self)
{
	self.flags &= ~(FL_FLY | FL_SWIM);
	self.monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self.item) {
		Drop_Item(self, self.item);
		self.item = 0;
	}

	if (self.deathtarget)
		self.target = self.deathtarget;

	if (!self.target)
		return;

	G_UseTargets(self, self.enemy);
}

//============================================================================

static bool monster_start(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return false;
	}

	if ((self.spawnflags & (spawn_flag)4) && !(self.monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self.spawnflags &= ~(spawn_flag)4;
		self.spawnflags |= (spawn_flag)1;
//      gi.dprintf("fixed spawnflags on %s at %s\n", self.classname, vtos(self.s.origin));
	}

	if (!(self.monsterinfo.aiflags & AI_GOOD_GUY_MASK))
		level.total_monsters++;

	self.nextthink = level.framenum + 1;
	self.svflags |= SVF_MONSTER;
	self.s.renderfx |= RF_FRAMELERP;
	self.takedamage = true;
	self.air_finished_framenum = level.framenum + 12 * BASE_FRAMERATE;
	self.use = monster_use_savable;
	self.max_health = self.health;
	self.clipmask = MASK_MONSTERSOLID;

	self.s.skinnum = 0;
	self.deadflag = DEAD_NO;
	self.svflags &= ~SVF_DEADMONSTER;

	if (!self.monsterinfo.checkattack)
		self.monsterinfo.checkattack = M_CheckAttack_savable;
	self.s.old_origin = self.s.origin;

	if (st.item)
	{
		self.item = FindItemByClassname(st.item);
		if (!self.item)
			gi.dprintf("%s at %s has bad item: %s\n", st.classname.ptr(), vtos(self.s.origin).ptr(), st.item.ptr());
	}

	// randomize what frame they start on
	if (self.monsterinfo.currentmove)
	{
		int first = self.monsterinfo.currentmove->firstframe;
		int last = self.monsterinfo.currentmove->lastframe;

		self.s.frame = first + (Q_rand() % (last - first + 1));
	}

#ifdef GROUND_ZERO
	// PMM - get this so I don't have to do it in all of the monsters
	self.monsterinfo.base_height = self.maxs[2];

	// PMM - clear these
	self.monsterinfo.quad_framenum = 0;
	self.monsterinfo.double_framenum = 0;
	self.monsterinfo.invincible_framenum = 0;
#endif

	return true;
}

static void monster_start_go(entity &self)
{
	vector	v;

	if (self.health <= 0)
		return;

	// check for target to combat_point and change to combattarget
	if (self.target)
	{
		bool	notcombat;
		bool	fixup;
		entityref ctarget = world;
		notcombat = false;
		fixup = false;
		while ((ctarget = G_FindFunc<&entity::targetname>(ctarget, self.target, striequals)).has_value())
		{
			if (ctarget->type == ET_POINT_COMBAT)
			{
				self.combattarget = self.target;
				fixup = true;
			}
			else
				notcombat = true;
		}

		if (notcombat && self.combattarget)
			gi.dprintf("%i at %s has target with mixed types\n", self.type, vtos(self.s.origin).ptr());
		if (fixup)
			self.target = nullptr;
	}

	// validate combattarget
	if (self.combattarget)
	{
		entityref	ctarget = world;
		while ((ctarget = G_FindFunc<&entity::targetname>(ctarget, self.combattarget, striequals)).has_value())
			if (ctarget->type != ET_POINT_COMBAT)
				gi.dprintf("%i at (%s) has a bad combattarget %s : %i at (%s)\n",
						   self.type, vtos(self.s.origin).ptr(),
						   self.combattarget.ptr(), ctarget->type, vtos(ctarget->s.origin).ptr());
	}

	if (self.target)
	{
		self.goalentity = self.movetarget = G_PickTarget(self.target);
		if (!self.movetarget.has_value())
		{
			gi.dprintf("%i can't find target %s at %s\n", self.type, self.target.ptr(), vtos(self.s.origin).ptr());
			self.target = nullptr;
			self.monsterinfo.pause_framenum = INT_MAX;
			self.monsterinfo.stand(self);
		}
		else if (self.movetarget->type == ET_PATH_CORNER)
		{
			v = self.goalentity->s.origin - self.s.origin;
			self.ideal_yaw = self.s.angles[YAW] = vectoyaw(v);
			self.monsterinfo.walk(self);
			self.target = nullptr;
		}
		else
		{
			self.goalentity = self.movetarget = world;
			self.monsterinfo.pause_framenum = INT_MAX;
			self.monsterinfo.stand(self);
		}
	}
	else
	{
		self.monsterinfo.pause_framenum = INT_MAX;
		self.monsterinfo.stand(self);
	}

	self.think = monster_think_savable;
	self.nextthink = level.framenum + 1;
}


static void walkmonster_start_go(entity &self)
{
	if (!(self.spawnflags & 2) && level.time < 1) {
		M_droptofloor(self);

		if (self.groundentity != null_entity)
			if (!M_walkmove(self, 0, 0))
				gi.dprintf("%i in solid at %s\n", self.type, vtos(self.s.origin).ptr());
	}

	if (!self.yaw_speed)
		self.yaw_speed = 20.f;

#ifdef GROUND_ZERO
	// PMM - stalkers are too short for this
	if (self.classname == "monster_stalker")
		self.viewheight = 15;
	else
#endif
		self.viewheight = 25;

	monster_start_go(self);

	if (self.spawnflags & (spawn_flag)2)
		monster_triggered_start(self);
}

REGISTER_SAVABLE_FUNCTION(walkmonster_start_go);

void walkmonster_start(entity &self)
{
	self.think = walkmonster_start_go_savable;
	monster_start(self);
}

static void flymonster_start_go(entity &self) 
{
	if (!M_walkmove(self, 0, 0))
		gi.dprintf("%i in solid at %s\n", self.type, vtos(self.s.origin).ptr());

	if (!self.yaw_speed)
		self.yaw_speed = 10.f;
	self.viewheight = 25;

	monster_start_go(self);

	if (self.spawnflags & (spawn_flag)2)
		monster_triggered_start(self);
}

REGISTER_SAVABLE_FUNCTION(flymonster_start_go);

void flymonster_start(entity &self)
{
	self.flags |= FL_FLY;
	self.think = flymonster_start_go_savable;
	monster_start(self);
}

static void swimmonster_start_go(entity &self)
{
	if (!self.yaw_speed)
		self.yaw_speed = 10.f;
	self.viewheight = 10;

	monster_start_go(self);

	if (self.spawnflags & (spawn_flag)2)
		monster_triggered_start(self);
}

REGISTER_SAVABLE_FUNCTION(swimmonster_start_go);

void swimmonster_start(entity &self)
{
	self.flags |= FL_SWIM;
	self.think = swimmonster_start_go_savable;
	monster_start(self);
}

#ifdef GROUND_ZERO
static void(entity self) stationarymonster_start_go;

void(entity self) stationarymonster_triggered_spawn =
{
	KillBox (self);

	self.solid = SOLID_BBOX;
	self.movetype = MOVETYPE_NONE;
	self.svflags &= ~SVF_NOCLIENT;
	self.air_finished_framenum = level.framenum + (12 * BASE_FRAMERATE);
	gi.linkentity (self);

	// FIXME - why doesn't this happen with real monsters?
	self.spawnflags &= ~2;

	stationarymonster_start_go (self);

	if (self.enemy && !(self.spawnflags & 1) && !(self.enemy.flags & FL_NOTARGET | FL_DISGUISED))
		FoundTarget (self);
	else
		self.enemy = 0;
}

static void(entity self, entity other, entity cactivator) stationarymonster_triggered_spawn_use =
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self.think = stationarymonster_triggered_spawn;
	self.nextthink = level.framenum + 1;
	if (cactivator.is_client)
		self.enemy = cactivator;
	self.use = monster_use;
}

void(entity self) stationarymonster_triggered_start =
{
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	self.svflags |= SVF_NOCLIENT;
	self.nextthink = 0;
	self.use = stationarymonster_triggered_spawn_use;
}

static void(entity self) stationarymonster_start_go =
{
	if (!self.yaw_speed)
		self.yaw_speed = 20f;

	monster_start_go (self);

	if (self.spawnflags & 2)
		stationarymonster_triggered_start (self);
}

void(entity self) stationarymonster_start =
{
	self.think = stationarymonster_start_go;
	monster_start (self);
}
#endif
#endif