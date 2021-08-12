#include "config.h"

#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
#include "game/entity.h"
#include "game/ballistics/blaster.h"
#include "game/rogue/ballistics/tracker.h"
#include "game/rogue/ballistics/heatbeam.h"
#include "game/misc.h"
#include "game/util.h"
#include "game/phys.h"
#include "monster.h"

void monster_fire_blaster2(entity &self, vector start, vector dir, int32_t damage, int32_t speed, monster_muzzleflash flashtype, entity_effects effect)
{
	fire_blaster(self, start, dir, damage, speed, EF_TRACKER | effect, false);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

void monster_fire_tracker(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity enemy, monster_muzzleflash flashtype)
{
	fire_tracker(self, start, dir, damage, speed, enemy);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

void monster_fire_heatbeam(entity &self, vector start, vector dir, int32_t damage, int32_t kick, monster_muzzleflash flashtype)
{
	fire_heatbeam(self, start, dir, damage, kick, true);

	gi.ConstructMessage(svc_muzzleflash2, self, flashtype).multicast(start, MULTICAST_PVS);
}

//
//ROGUE
//

//
// Monster spawning code
//
// Used by the carrier, the medic_commander, and the black widow
//
// The sequence to create a flying monster is:
//
//  FindSpawnPoint - tries to find suitable spot to spawn the monster in
//  CreateFlyMonster  - this verifies the point as good and creates the monster

// To create a ground walking monster:
//
//  FindSpawnPoint - same thing
//  CreateGroundMonster - this checks the volume and makes sure the floor under the volume is suitable
//

//
// CreateMonster
//
entity &CreateMonster(vector origin, vector angles, const entity_type &type)
{
	entity &new_ent = G_Spawn();

	new_ent.origin = origin;
	new_ent.angles = angles;
	new_ent.monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	new_ent.type = type;

	ED_CallSpawn(new_ent);

	new_ent.renderfx |= RF_IR_VISIBLE;

	return new_ent;
}

//
// CheckSpawnPoint
//
// PMM - checks volume to make sure we can spawn a monster there (is it solid?)
//
// This is all fliers should need
bool CheckSpawnPoint(vector origin, vector mins, vector maxs)
{
	if (!mins || !maxs)
		return false;

	trace tr = gi.trace(origin, { mins, maxs }, origin, 0, MASK_MONSTERSOLID);

	if (tr.startsolid || tr.allsolid)
		return false;

	if (!tr.ent.is_world())
		return false;

	return true;
}

//
// CheckGroundSpawnPoint
//
// PMM - used for walking monsters
//  checks:
//		1)	is there a ground within the specified height of the origin?
//		2)	is the ground non-water?
//		3)	is the ground flat enough to walk on?
//
bool CheckGroundSpawnPoint(vector origin, vector ent_mins, vector ent_maxs, float height, float gravity)
{
	if (!CheckSpawnPoint (origin, ent_mins, ent_maxs))
		return false;

	vector stop = origin;
	stop[2] = origin[2] + ent_mins[2] - height;

	trace tr = gi.trace (origin, { ent_mins, ent_maxs }, stop, 0, MASK_MONSTERSOLID | MASK_WATER);

	// it's not going to be all solid or start solid, since that's checked above
	if ((tr.fraction < 1) && (tr.contents & MASK_MONSTERSOLID))
	{
		// we found a non-water surface down there somewhere.  now we need to check to make sure it's not too sloped
		//
		// algorithm straight out of m_move.c:M_CheckBottom()
		//

		// first, do the midpoint trace
		vector mins = tr.endpos + ent_mins;
		vector maxs = tr.endpos + ent_maxs;

		// first, do the easy flat check
		vector start;

		if (gravity > 0)
			start[2] = maxs[2] + 1;
		else
			start[2] = mins[2] - 1;

		for (int x = 0; x <= 1; x++)
			for (int y = 0; y <= 1; y++)
			{
				start[0] = x ? maxs[0] : mins[0];
				start[1] = y ? maxs[1] : mins[1];
				if (gi.pointcontents (start) != CONTENTS_SOLID)
					goto realcheck;
			}

		// if it passed all four above checks, we're done
		return true;

realcheck:
		// check it for real
		start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
		start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
		start[2] = mins[2];

		tr = gi.traceline(start, stop, 0, MASK_MONSTERSOLID);

		if (tr.fraction == 1.0)
			return false;

		float mid, bottom;

		if (gravity < 0)
		{
			start[2] = mins[2];
			stop[2] = start[2] - STEPSIZE - STEPSIZE;
			mid = bottom = tr.endpos[2] + ent_mins[2];
		}
		else
		{
			start[2] = maxs[2];
			stop[2] = start[2] + STEPSIZE + STEPSIZE;
			mid = bottom = tr.endpos[2] - ent_maxs[2];
		}

		for (int x = 0; x <= 1; x++)
			for (int y = 0; y <= 1; y++)
			{
				start[0] = stop[0] = x ? maxs[0] : mins[0];
				start[1] = stop[1] = y ? maxs[1] : mins[1];

				tr = gi.traceline(start, stop, 0, MASK_MONSTERSOLID);

				if(gravity > 0)
				{
					if (tr.fraction != 1.0 && tr.endpos[2] < bottom)
						bottom = tr.endpos[2];
					if (tr.fraction == 1.0 || tr.endpos[2] - mid > STEPSIZE)
						return false;
				}
				else
				{
					if (tr.fraction != 1.0 && tr.endpos[2] > bottom)
						bottom = tr.endpos[2];
					if (tr.fraction == 1.0 || mid - tr.endpos[2] > STEPSIZE)
						return false;
				}
			}

		return true; // we can land on it, it's ok
	}

	// otherwise, it's either water (bad) or not there (too far)
	// if we're here, it's bad below
	return false;
}

// This is just a wrapper for CreateMonster that looks down height # of CMUs and sees if there
// are bad things down there or not
//
// this is from m_move.c
entityref CreateGroundMonster(vector origin, vector angles, vector mins, vector maxs, const entity_type &type, int32_t height)
{
	// check the ground to make sure it's there, it's relatively flat, and it's not toxic
	if (!CheckGroundSpawnPoint(origin, mins, maxs, height, -1))
		return 0;

	return CreateMonster (origin, angles, type);
}

// FindSpawnPoint
// PMM - this is used by the medic commander (possibly by the carrier) to find a good spawn point
// if the startpoint is bad, try above the startpoint for a bit
bool FindSpawnPoint(vector startpoint, vector mins, vector maxs, vector &spawnpoint, float max_move_up)
{
	trace tr = gi.trace (startpoint, { mins, maxs }, startpoint, 0, MASK_MONSTERSOLID|CONTENTS_PLAYERCLIP);

	if (tr.startsolid || tr.allsolid || !tr.ent.is_world())
	{
		vector top = startpoint;
		top[2] += max_move_up;

		tr = gi.trace (top, { mins, maxs }, startpoint, 0, MASK_MONSTERSOLID);
		if (tr.startsolid || tr.allsolid)
			return false;

		spawnpoint = tr.endpos;
		return true;
	}
	
	spawnpoint = startpoint;
	return true;
}

// ****************************
// SPAWNGROW stuff
// ****************************
constexpr float SPAWNGROW_LIFESPAN		= 0.3f;

static void spawngrow_think(entity &self)
{
	self.angles = randomv({ 360, 360, 360 });

	if ((level.framenum < self.wait) && (self.frame < 2))
		self.frame++;
	if (level.framenum >= self.wait)
	{
		if (self.effects & EF_SPHERETRANS)
		{
			G_FreeEdict (self);
			return;
		}
		else if (self.frame > 0)
			self.frame--;
		else
		{
			G_FreeEdict (self);
			return;
		}
	}

	self.nextthink += 1;
}

REGISTER_STATIC_SAVABLE(spawngrow_think);

void SpawnGrow_Spawn(vector startpos, int32_t size)
{
	entity &ent = G_Spawn();
	ent.origin = startpos;
	ent.angles = randomv({ 360, 360, 360 });	
	ent.solid = SOLID_NOT;
	ent.renderfx = RF_IR_VISIBLE;
	ent.movetype = MOVETYPE_NONE;

	float lifespan = SPAWNGROW_LIFESPAN;

	if (size <= 1)
		ent.modelindex = gi.modelindex("models/items/spawngro2/tris.md2");
	else if (size == 2)
	{
		ent.modelindex = gi.modelindex("models/items/spawngro3/tris.md2");
		lifespan = 2.f;
	}
	else
		ent.modelindex = gi.modelindex("models/items/spawngro/tris.md2");

	ent.think = SAVABLE(spawngrow_think);

	ent.wait = level.framenum + (lifespan * BASE_FRAMERATE);
	ent.nextthink = level.framenum + 1;
	if (size != 2)
		ent.effects |= EF_SPHERETRANS;
	gi.linkentity (ent);
}


// ****************************
// WidowLeg stuff
// ****************************

inline vector WidowVelocityForDamage(int32_t damage)
{
	return { damage * crandom(), damage * crandom(), damage * crandom() + 200.f };
}

static void widow_gib_touch(entity &self, entity &, vector, const surface &)
{
	self.solid = SOLID_NOT;
	self.touch = nullptr;
	self.angles[PITCH] = 0;
	self.angles[ROLL] = 0;
	self.avelocity = vec3_origin;

	if (self.moveinfo.sound_start)
		gi.sound (self, CHAN_VOICE, self.moveinfo.sound_start);
}

REGISTER_STATIC_SAVABLE(widow_gib_touch);

void ThrowWidowGibReal(entity &self, stringlit gibname, int32_t damage, gib_type type, vector startpos, bool sized, sound_index hitsound, bool fade)
{
	if (!gibname)
		return;

	entity &gib = G_Spawn();

	if (startpos)
		gib.origin = startpos;
	else
	{
		vector csize = self.size * 0.5f;
		vector origin = self.bounds.mins + csize;
		gib.origin = origin + randomv(-csize, csize);
	}

	gib.solid = SOLID_NOT;
	gib.effects |= EF_GIB;
	gib.flags |= FL_NO_KNOCKBACK;
	gib.takedamage = true;
	gib.die = SAVABLE(gib_die);
	gib.renderfx |= RF_IR_VISIBLE;
	gib.think = SAVABLE(G_FreeEdict);

	if (fade)
	{
		// sized gibs last longer
		if (sized)
			gib.nextthink = level.framenum + (gtime)(random(20.f, 35.f) * BASE_FRAMERATE);
		else
			gib.nextthink = level.framenum + (gtime)(random(5.f, 15.f) * BASE_FRAMERATE);
	}
	else
	{
		// sized gibs last longer
		if (sized)
			gib.nextthink = level.framenum + (gtime)(random(60.f, 75.f) * BASE_FRAMERATE);
		else
			gib.nextthink = level.framenum + (gtime)(random(25.f, 35.f) * BASE_FRAMERATE);
	}

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

	vector vd = WidowVelocityForDamage (damage);
	gib.velocity = self.velocity + (vscale * vd);
	ClipGibVelocity (gib);

	gi.setmodel (gib, gibname);

	if (sized)
	{
		gib.moveinfo.sound_start = hitsound;
		gib.solid = SOLID_BBOX;
		gib.avelocity = randomv({ 400.f, 400.f, 200.f });
		if (gib.velocity[2] < 0)
			gib.velocity[2] *= -1;
		gib.velocity[0] *= 2;
		gib.velocity[1] *= 2;
		ClipGibVelocity (gib);
		gib.velocity[2] = max(random(350.f, 450.f), gib.velocity[2]);
		gib.gravity = 0.25;
		gib.touch = SAVABLE(widow_gib_touch);
		gib.owner = self;

		if (gib.modelindex == gi.modelindex ("models/monsters/blackwidow2/gib2/tris.md2"))
		{
			gib.bounds = {
				.mins = { -10, -10, 0 },
				.maxs = { 10, 10, 10 }
			};
		}
		else
		{
			gib.bounds = {
				.mins = { -5, -5, 0 },
				.maxs = { 5, 5, 5 }
			};
		}
	}
	else
	{
		gib.velocity[0] *= 2;
		gib.velocity[1] *= 2;
		gib.avelocity = randomv({ 600.f, 600.f, 600.f });
	}

	gi.linkentity(gib);
}

inline void ThrowWidowGib(entity &self, stringlit gibname, int32_t damage, gib_type type)
{
	ThrowWidowGibReal(self, gibname, damage, type, vec3_origin, false, SOUND_NONE, true);
}

inline void ThrowWidowGibLoc(entity &self, stringlit gibname, int32_t damage, gib_type type, vector startpos, bool fade)
{
	ThrowWidowGibReal(self, gibname, damage, type, startpos, false, SOUND_NONE, fade);
}

inline void ThrowWidowGibSized(entity &self, stringlit gibname, int32_t damage, gib_type type, vector startpos, sound_index hitsound, bool fade)
{
	ThrowWidowGibReal(self, gibname, damage, type, startpos, true, hitsound, fade);
}

inline void ThrowSmallStuff(entity &self, vector point)
{
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point, false);
}

inline void ThrowMoreStuff(entity &self, vector point)
{
	if (coop)
	{
		ThrowSmallStuff (self, point);
		return;
	}

	ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point, false);
}

constexpr int32_t MAX_LEGSFRAME	= 23;
constexpr float LEG_WAIT_TIME	= 1.f;

static void widowlegs_think(entity &self)
{
	if (self.frame == 17)
	{
		vector f, r, u;
		AngleVectors (self.angles, &f, &r, &u);

		vector start = G_ProjectSource(self.origin, { 11.77f, -7.24f, 23.31f }, f, r, u);
		gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1, start).multicast(start, MULTICAST_ALL);
		ThrowSmallStuff (self, start);
	}

	if (self.frame < MAX_LEGSFRAME)
	{
		self.frame++;
		self.nextthink = level.framenum + 1;
		return;
	}
	else if (self.wait == 0)
		self.wait = level.framenum + (LEG_WAIT_TIME * BASE_FRAMERATE);

	if (level.framenum > self.wait)
	{
		vector f, r, u;
		AngleVectors (self.angles, &f, &r, &u);

		vector start = G_ProjectSource(self.origin, { -65.6f, -8.44f, 28.59f }, f, r, u);
		gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1, start).multicast(start, MULTICAST_ALL);
		ThrowSmallStuff (self, start);

		ThrowWidowGibSized (self, "models/monsters/blackwidow/gib1/tris.md2", (int32_t) random(80.f, 100.f), GIB_METALLIC, start, SOUND_NONE, true);
		ThrowWidowGibSized (self, "models/monsters/blackwidow/gib2/tris.md2", (int32_t) random(80.f, 100.f), GIB_METALLIC, start, SOUND_NONE, true);

		start = G_ProjectSource(self.origin, { -1.04f, -51.18f, 7.04f }, f, r, u);
		gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1, start).multicast(start, MULTICAST_ALL);
		ThrowSmallStuff (self, start);

		ThrowWidowGibSized (self, "models/monsters/blackwidow/gib1/tris.md2", (int32_t) random(80.f, 100.f), GIB_METALLIC, start, SOUND_NONE, true);
		ThrowWidowGibSized (self, "models/monsters/blackwidow/gib2/tris.md2", (int32_t) random(80.f, 100.f), GIB_METALLIC, start, SOUND_NONE, true);
		ThrowWidowGibSized (self, "models/monsters/blackwidow/gib3/tris.md2", (int32_t) random(80.f, 100.f), GIB_METALLIC, start, SOUND_NONE, true);

		G_FreeEdict (self);
		return;
	}

	if ((level.framenum > (self.wait - (0.5 * BASE_FRAMERATE))) && (self.count == 0))
	{
		self.count = 1;

		vector f, r, u;
		AngleVectors (self.angles, &f, &r, &u);

		vector start = G_ProjectSource(self.origin, { 31, -88.7f, 10.96f }, f, r, u);
		gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1, start).multicast(start, MULTICAST_ALL);

		start = G_ProjectSource(self.origin, { -12.67f, -4.39f, 15.68f }, f, r, u);
		gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1, start).multicast(start, MULTICAST_ALL);
	}

	self.nextthink = level.framenum + 1;
}

REGISTER_STATIC_SAVABLE(widowlegs_think);

void Widowlegs_Spawn(vector startpos, vector cangles)
{
	entity &ent = G_Spawn();
	ent.origin = startpos;
	ent.angles = cangles;
	ent.solid = SOLID_NOT;
	ent.renderfx = RF_IR_VISIBLE;
	ent.movetype = MOVETYPE_NONE;

	ent.modelindex = gi.modelindex("models/monsters/legs/tris.md2");
	ent.think = SAVABLE(widowlegs_think);

	ent.nextthink = level.framenum + 1;
	gi.linkentity (ent);
}

inline void ThrowArm1(entity &self)
{
	vector	f, r, u;
	AngleVectors (self.angles, &f, &r, &u);

	vector startpoint = G_ProjectSource(self.origin, { 65.76f, 17.52f, 7.56f }, f, r, u);

	gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1_BIG, startpoint).multicast(self.origin, MULTICAST_ALL);

	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
}

inline void ThrowArm2(entity &self)
{
	vector f, r, u;
	AngleVectors (self.angles, &f, &r, &u);

	vector startpoint = G_ProjectSource(self.origin, { 65.76f, 17.52f, 7.56f }, f, r, u);

	ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib4/tris.md2", 200, GIB_METALLIC, startpoint, gi.soundindex ("misc/fhit3.wav"), false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
}

template<float x, float y, float z>
inline void WidowExplosion(entity &self)
{
	vector f, r, u;
	AngleVectors (self.angles, &f, &r, &u);

	vector startpoint = G_ProjectSource(self.origin, { x, y, z }, f, r, u);

	gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1, startpoint).multicast(self.origin, MULTICAST_ALL);

	ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
}

constexpr auto WidowExplosion1 = WidowExplosion<23.74f, -37.67f, 76.96f>;
constexpr auto WidowExplosion2 = WidowExplosion<-20.49f, 36.92f, 73.52f>;
constexpr auto WidowExplosion3 = WidowExplosion<2.11f, 0.05f, 92.20f>;
constexpr auto WidowExplosion4 = WidowExplosion<-28.04f, -35.57f, -77.56f>;
constexpr auto WidowExplosion5 = WidowExplosion<-20.11f, -1.11f, 40.76f>;

inline void WidowExplosionLeg(entity self)
{
	vector	f, r, u;
	AngleVectors (self.angles, &f, &r, &u);

	vector startpoint = G_ProjectSource(self.origin, { -31.89f, -47.86f, 67.02f }, f, r, u);

	gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1_BIG, startpoint).multicast(self.origin, MULTICAST_ALL);

	ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib2/tris.md2", 200, GIB_METALLIC, startpoint, gi.soundindex ("misc/fhit3.wav"), false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);

	startpoint = G_ProjectSource(self.origin, { -44.9f, -82.14f, 54.72f }, f, r, u);

	gi.ConstructMessage(svc_temp_entity, TE_EXPLOSION1, startpoint).multicast(self.origin, MULTICAST_ALL);

	ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib1/tris.md2", 300, GIB_METALLIC, startpoint, gi.soundindex ("misc/fhit3.wav"), false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
}

#endif