#include "../lib/types.h"
#include "entity.h"
#include "../lib/gi.h"
#include "combat.h"
#include "util.h"
#include "cmds.h"
#include "misc.h"

#ifdef SINGLE_PLAYER
#include "pweapon.h"

void check_dodge(entity &self, vector start, vector dir, int32_t speed)
{
	vector	end;
	vector	v;
	trace	tr;
	float	eta;

	// easy mode only ducks one quarter the time
	if ((int32_t)skill == 0) {
		if (random() > 0.25f)
			return;
	}
	end = start + (8192 * dir);
	tr = gi.traceline(start, end, self, MASK_SHOT);
	if (!tr.ent.is_world() && (tr.ent.svflags & SVF_MONSTER) && (tr.ent.health > 0) && tr.ent.monsterinfo.dodge && infront(tr.ent, self)) {
		v = tr.endpos - start;
		eta = (VectorLength(v) - tr.ent.maxs.x) / speed;
		tr.ent.monsterinfo.dodge(tr.ent, self, eta
#ifdef GROUND_ZERO
			, &tr
#endif
		);
	}
}

bool fire_hit(entity &self, vector aim, int32_t damage, int32_t kick)
{
	trace	tr;
	vector	forward, right, up;
	vector	v;
	vector	point;
	float	range;
	vector	dir;

	//see if enemy is in range
	dir = self.enemy->s.origin - self.s.origin;
	range = VectorLength(dir);

	if (range > aim.x)
		return false;

	if (aim.y > self.mins.x && aim.y < self.maxs.x)
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self.enemy->maxs.x;
	// this is a side hit so adjust the "right" value out to the edge of their bbox
	else if (aim.y < 0)
		aim.y = self.enemy->mins.x;
	else
		aim.y = self.enemy->maxs.x;

	point = self.s.origin + (range * dir);

	tr = gi.traceline(self.s.origin, point, self, MASK_SHOT);

	entityref hit_entity = tr.ent;

	if (tr.fraction < 1)
	{
		if (!tr.ent.takedamage)
			return false;
		// if it will hit any client/monster then hit the one we wanted to hit
		if ((tr.ent.svflags & SVF_MONSTER) || (tr.ent.is_client()))
			hit_entity = self.enemy;
	}

	AngleVectors(self.s.angles, &forward, &right, &up);
	point = self.s.origin + (range * forward);
	point = point + (aim.y * right);
	point = point + (aim.z * up);
	dir = point - self.enemy->s.origin;

	// do the damage
	T_Damage(hit_entity, self, self, dir, point, vec3_origin, damage, kick / 2, DAMAGE_NO_KNOCKBACK, MOD_HIT);

	if (!(hit_entity->svflags & SVF_MONSTER) && (!hit_entity->is_client()))
		return false;

	// do our special form of knockback here
	v = self.enemy->absmin + (0.5f * self.enemy->size);
	v -= point;
	VectorNormalize(v);
	self.enemy->velocity += (kick * v);
	if (self.enemy->velocity.z > 0)
		self.enemy->groundentity = null_entity;
	return true;
}
#endif

/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
static inline void fire_lead(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, temp_event te_impact, int32_t hspread, int32_t vspread, means_of_death mod)
{
	trace	tr;
	vector	dir;
	vector	forward, right, up;
	vector	end;
	float	r;
	float	u;
	vector	water_start = vec3_origin;
	bool	water = false;
	content_flags	content_mask = MASK_SHOT | MASK_WATER;
	
	tr = gi.traceline(self.s.origin, start, self, MASK_SHOT);
	if (!(tr.fraction < 1.0f))
	{
		dir = vectoangles(aimdir);

		AngleVectors(dir, &forward, &right, &up);

		r = crandom() * hspread;
		u = crandom() * vspread;
		end = start + (8192 * forward);
		end += (r * right);
		end += (u * up);

		if (gi.pointcontents(start) & MASK_WATER)
		{
			water = true;
			water_start = start;
			content_mask &= ~MASK_WATER;
		}

		tr = gi.traceline(start, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			splash_type	color;

			water = true;
			water_start = tr.endpos;

			if (start != tr.endpos)
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (tr.surface.name == "*brwater")
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_SPLASH);
					gi.WriteByte(8);
					gi.WritePosition(tr.endpos);
					gi.WriteDir(tr.normal);
					gi.WriteByte(color);
					gi.multicast(tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				dir = end - start;
				dir = vectoangles(dir);
				AngleVectors(dir, &forward, &right, &up);
				r = crandom() * hspread * 2;
				u = crandom() * vspread * 2;
				end = water_start + (8192 * forward);
				end += (r * right);
				end += (u * up);
			}

			// re-trace ignoring water this time
			tr = gi.traceline(water_start, end, self, MASK_SHOT);
		}
	}

	// send gun puff / flash
	if (!(tr.surface.flags & SURF_SKY))
	{
		if (tr.fraction < 1.0f)
		{
			if (tr.ent.takedamage)
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.normal, damage, kick, DAMAGE_BULLET, mod);
			else if (strncmp(tr.surface.name.data(), "sky", 3) != 0)
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(te_impact);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.normal);
				gi.multicast(tr.endpos, MULTICAST_PVS);

#ifdef SINGLE_PLAYER
				if (self.is_client())
					PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
#endif
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vector	pos;

		dir = tr.endpos - water_start;
		VectorNormalize(dir);
		pos = tr.endpos + (-2 * dir);
		if (gi.pointcontents(pos) & MASK_WATER)
			tr.endpos = pos;
		else
			tr = gi.traceline(pos, water_start, tr.ent, MASK_WATER);

		pos = (water_start + tr.endpos) * 0.5f;

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BUBBLETRAIL);
		gi.WritePosition(water_start);
		gi.WritePosition(tr.endpos);
		gi.multicast(pos, MULTICAST_PVS);
	}
}

void fire_bullet(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, means_of_death mod)
{
	fire_lead(self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);
}

void fire_shotgun(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, int32_t count, means_of_death mod)
{
	for (int32_t i = 0; i < count; i++)
		fire_lead(self, start, aimdir, damage, kick, TE_SHOTGUN, hspread, vspread, mod);
}

constexpr spawn_flag BLASTER_IS_HYPER = (spawn_flag)1;

/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
static void blaster_touch(entity &self, entity &other, vector normal, const surface &surf)
{
	if (other == self.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(self);
		return;
	}
#ifdef SINGLE_PLAYER

	if (self.owner->is_client())
		PlayerNoise(self.owner, self.s.origin, PNOISE_IMPACT);
#endif

	if (other.takedamage)
		T_Damage(other, self, self.owner, self.velocity, self.s.origin, normal, self.dmg, 1, DAMAGE_ENERGY, (means_of_death)self.sounds);
	else
	{
		gi.WriteByte(svc_temp_entity);
#ifdef THE_RECKONING
		if (self.s.effects & EF_BLUEHYPERBLASTER)	// Knightmare- this was checking bit TE_BLUEHYPERBLASTER
			gi.WriteByte (TE_FLECHETTE);			// Knightmare- TE_BLUEHYPERBLASTER is broken (parse error) in most Q2 engines
		else
#endif
			gi.WriteByte(TE_BLASTER);
		gi.WritePosition(self.s.origin);
		gi.WriteDir(normal);
		gi.multicast(self.s.origin, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

REGISTER_SAVABLE_FUNCTION(blaster_touch);

void fire_blaster(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity_effects effect, means_of_death mod, bool hyper)
{
	VectorNormalize(dir);

	entity &bolt = G_Spawn();
	bolt.svflags = SVF_DEADMONSTER;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	bolt.s.origin = start;
	bolt.s.old_origin = start;
	bolt.s.angles = vectoangles(dir);
	bolt.velocity = dir * speed;
	bolt.movetype = MOVETYPE_FLYMISSILE;
	bolt.clipmask = MASK_SHOT;
	bolt.solid = SOLID_BBOX;
	bolt.s.effects |= effect;
	bolt.mins = vec3_origin;
	bolt.maxs = vec3_origin;
#ifdef THE_RECKONING
	if (effect & EF_BLUEHYPERBLASTER)
		bolt.s.modelindex = gi.modelindex("models/objects/blaser/tris.md2");
	else
#endif
		bolt.s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	bolt.s.sound = gi.soundindex("misc/lasfly.wav");
	bolt.owner = self;
	bolt.touch = blaster_touch_savable;
	bolt.nextthink = level.framenum + 2 * BASE_FRAMERATE;
	bolt.think = G_FreeEdict_savable;
	bolt.dmg = damage;
	bolt.type = ET_BLASTER_BOLT;
	if (hyper)
		bolt.spawnflags = BLASTER_IS_HYPER;
	bolt.sounds = mod;
	gi.linkentity(bolt);

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge(self, bolt.s.origin, dir, speed);
#endif

	trace tr = gi.traceline(self.s.origin, bolt.s.origin, bolt, MASK_SHOT);
	
	if (tr.fraction < 1.0f)
	{
		bolt.s.origin += (-10 * dir);
		bolt.touch(bolt, tr.ent, vec3_origin, null_surface);
	}
}

constexpr spawn_flag GRENADE_IS_HAND = (spawn_flag)1;
constexpr spawn_flag GRENADE_IS_HELD = (spawn_flag)2;

/*
=================
fire_grenade
=================
*/
void Grenade_Explode(entity &ent)
{
	vector			origin;
	means_of_death	mod;
#ifdef SINGLE_PLAYER

	if (ent.owner->is_client())
		PlayerNoise(ent.owner, ent.s.origin, PNOISE_IMPACT);
#endif

	//FIXME: if we are onground then raise our Z just a bit since we are a point?
	if (ent.enemy.has_value())
	{
		float	points;
		vector	v;
		vector dir;

		v = ent.enemy->mins + ent.enemy->maxs;
		v = ent.enemy->s.origin + (0.5f * v);
		v = ent.s.origin - v;
		points = ent.dmg - 0.5f * VectorLength(v);
		dir = ent.enemy->s.origin - ent.s.origin;
		if (ent.spawnflags & GRENADE_IS_HAND)
			mod = MOD_HANDGRENADE;
		else
			mod = MOD_GRENADE;
		T_Damage(ent.enemy, ent, ent.owner, dir, ent.s.origin, vec3_origin, (int32_t)points, (int32_t)points, DAMAGE_RADIUS, mod);
	}

	if (ent.spawnflags & GRENADE_IS_HELD)
		mod = MOD_HELD_GRENADE;
	else if (ent.spawnflags & GRENADE_IS_HAND)
		mod = MOD_HG_SPLASH;
	else
		mod = MOD_G_SPLASH;
	T_RadiusDamage(ent, ent.owner, (float)ent.dmg, ent.enemy, ent.dmg_radius, mod);

	origin = ent.s.origin + (-0.02f * ent.velocity);
	gi.WriteByte(svc_temp_entity);
	if (ent.waterlevel)
	{
		if (ent.groundentity.has_value())
			gi.WriteByte(TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	}
	else
	{
		if (ent.groundentity.has_value())
			gi.WriteByte(TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION);
	}
	gi.WritePosition(origin);
	gi.multicast(ent.s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

REGISTER_SAVABLE_FUNCTION(Grenade_Explode);

static void Grenade_Touch(entity &ent, entity &other, vector, const surface &surf)
{
	if (other == ent.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(ent);
		return;
	}

	if (!other.takedamage)
	{
		if (ent.spawnflags & 1)
		{
			if (random() > 0.5f)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
		}
		else
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);

		return;
	}

	ent.enemy = other;
	Grenade_Explode(ent);
}

REGISTER_SAVABLE_FUNCTION(Grenade_Touch);

void fire_grenade(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, float timer, float damage_radius)
{
	vector	dir;
	vector	forward, right, up;
	float   scale;

	dir = vectoangles(aimdir);
	AngleVectors(dir, &forward, &right, &up);

	entity &grenade = G_Spawn();
	grenade.s.origin = start;
	grenade.velocity = aimdir * speed;
	scale = random(190.f, 210.f);
	grenade.velocity += (scale * up);
	scale = random(-10.f, 10.f);
	grenade.velocity += (scale * right);
	grenade.avelocity = { 300, 300, 300 };
	grenade.movetype = MOVETYPE_BOUNCE;
	grenade.clipmask = MASK_SHOT;
	grenade.solid = SOLID_BBOX;
	grenade.s.effects |= EF_GRENADE;
	grenade.mins = vec3_origin;
	grenade.maxs = vec3_origin;
	grenade.s.modelindex = gi.modelindex("models/objects/grenade/tris.md2");
	grenade.owner = self;
	grenade.touch = Grenade_Touch_savable;
	grenade.nextthink = level.framenum + (gtime)(timer * BASE_FRAMERATE);
	grenade.think = Grenade_Explode_savable;
	grenade.dmg = damage;
	grenade.dmg_radius = damage_radius;
	grenade.type = ET_GRENADE;

	gi.linkentity(grenade);
}

void fire_grenade2(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, float timer, float damage_radius, bool held)
{
	vector	dir;
	vector	forward, right, up;
	float   scale;

	dir = vectoangles(aimdir);
	AngleVectors(dir, &forward, &right, &up);

	entity &grenade = G_Spawn();
	grenade.s.origin = start;
	grenade.velocity = aimdir * speed;
	scale = random(190.f, 210.f);
	grenade.velocity += (scale * up);
	scale = random(-10.f, 10.f);
	grenade.velocity += (scale * right);
	grenade.avelocity = { 300, 300, 300 };
	grenade.movetype = MOVETYPE_BOUNCE;
	grenade.clipmask = MASK_SHOT;
	grenade.solid = SOLID_BBOX;
	grenade.s.effects |= EF_GRENADE;
	grenade.mins = vec3_origin;
	grenade.maxs = vec3_origin;
	grenade.s.modelindex = gi.modelindex("models/objects/grenade2/tris.md2");
	grenade.owner = self;
	grenade.touch = Grenade_Touch_savable;
	grenade.nextthink = level.framenum + (gtime)(timer * BASE_FRAMERATE);
	grenade.think = Grenade_Explode_savable;
	grenade.dmg = damage;
	grenade.dmg_radius = damage_radius;
	grenade.type = ET_HANDGRENADE;
	grenade.spawnflags = GRENADE_IS_HAND;
	if (held)
		grenade.spawnflags |= GRENADE_IS_HELD;
	grenade.s.sound = gi.soundindex("weapons/hgrenc1b.wav");

	if (timer <= 0.0f)
		Grenade_Explode(grenade);
	else
	{
		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
		gi.linkentity(grenade);
	}
}

/*
=================
fire_rocket
=================
*/
static void rocket_touch(entity &ent, entity &other, vector normal, const surface &surf)
{
	vector	origin;

	if (other == ent.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(ent);
		return;
	}
#ifdef SINGLE_PLAYER

	if (ent.owner->is_client())
		PlayerNoise(ent.owner, ent.s.origin, PNOISE_IMPACT);
#endif

	// calculate position for the explosion entity
	origin = ent.s.origin + (-0.02f * ent.velocity);

	if (other.takedamage)
		T_Damage(other, ent, ent.owner, ent.velocity, ent.s.origin, normal, ent.dmg, 0, DAMAGE_NONE, MOD_ROCKET);
#ifdef SINGLE_PLAYER
	// don't throw any debris in net games
	else if (!deathmatch && !coop)
	{
		if (!(surf.flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66 | SURF_FLOWING)))
		{
			int32_t n = Q_rand() % 5;
			while (n--)
				ThrowDebris(ent, "models/objects/debris2/tris.md2", 2, ent.s.origin);
		}
	}
#endif

	T_RadiusDamage(ent, ent.owner, (float)ent.radius_dmg, other, ent.dmg_radius, MOD_R_SPLASH);

	gi.WriteByte(svc_temp_entity);
	if (ent.waterlevel)
		gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(origin);
	gi.multicast(ent.s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

REGISTER_SAVABLE_FUNCTION(rocket_touch);

entity &fire_rocket(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius, int radius_damage)
{
	entity &rocket = G_Spawn();
	rocket.s.origin = start;
	rocket.s.angles = vectoangles(dir);
	rocket.velocity = dir * speed;
	rocket.movetype = MOVETYPE_FLYMISSILE;
	rocket.clipmask = MASK_SHOT;
	rocket.solid = SOLID_BBOX;
	rocket.s.effects |= EF_ROCKET;
	rocket.mins = vec3_origin;
	rocket.maxs = vec3_origin;
	rocket.s.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
	rocket.owner = self;
	rocket.touch = rocket_touch_savable;
	rocket.nextthink = level.framenum + BASE_FRAMERATE * 8000 / speed;
	rocket.think = G_FreeEdict_savable;
	rocket.dmg = damage;
	rocket.radius_dmg = radius_damage;
	rocket.dmg_radius = damage_radius;
	rocket.s.sound = gi.soundindex("weapons/rockfly.wav");
	rocket.type = ET_ROCKET;

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge(self, rocket.s.origin, dir, speed);
#endif

	gi.linkentity(rocket);
	return rocket;
}

/*
=================
fire_rail
=================
*/
void fire_rail(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick)
{
	if (gi.pointcontents(start) & MASK_SOLID)
		return;

	vector	from = start;
	vector	end = start + (8192 * aimdir);
	entityref	ignore = self;
	content_flags	mask = MASK_SHOT | CONTENTS_SLIME | CONTENTS_LAVA;
	bool	water = false;
	trace	tr;

	while (ignore.has_value())
	{
		tr = gi.traceline(from, end, ignore, mask);

		if (tr.contents & (CONTENTS_SLIME | CONTENTS_LAVA))
		{
			mask &= ~(CONTENTS_SLIME | CONTENTS_LAVA);
			water = true;
		}
		else
		{
			//ZOID--added so rail goes through SOLID_BBOX entities (gibs, etc)
			if ((tr.ent.svflags & SVF_MONSTER) || tr.ent.is_client() ||
				(tr.ent.solid == SOLID_BBOX))
				ignore = tr.ent;
			else
				ignore = nullptr;

			if ((tr.ent != self) && (tr.ent.takedamage))
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.normal, damage, kick, DAMAGE_NONE, MOD_RAILGUN);
		}

		from = tr.endpos;
	}

	// send gun puff / flash
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_RAILTRAIL);
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(self.s.origin, MULTICAST_PHS);
	if (water)
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_RAILTRAIL);
		gi.WritePosition(start);
		gi.WritePosition(tr.endpos);
		gi.multicast(tr.endpos, MULTICAST_PHS);
	}
#ifdef SINGLE_PLAYER

	if (self.is_client())
		PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
#endif
}

/*
=================
fire_bfg
=================
*/
static void bfg_explode(entity &self)
{
	entityref	ent;
	float	points;
	vector	v;
	float	dist;

	if (self.s.frame == 0)
	{
		// the BFG effect
		ent = world;
		while ((ent = findradius(ent, self.s.origin, self.dmg_radius)).has_value())
		{
			if (!ent->takedamage)
				continue;
			if (ent == self.owner)
				continue;
			if (!CanDamage(ent, self))
				continue;
			if (!CanDamage(ent, self.owner))
				continue;

			v = ent->mins + ent->maxs;
			v = ent->s.origin + (0.5f * v);
			v = self.s.origin - v;
			dist = VectorLength(v);
			points = self.radius_dmg * (1.0f - sqrt(dist / self.dmg_radius));
			if (ent == self.owner)
				points *= 0.5f;

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BFG_EXPLOSION);
			gi.WritePosition(ent->s.origin);
			gi.multicast(ent->s.origin, MULTICAST_PHS);
			T_Damage(ent, self, self.owner, self.velocity, ent->s.origin, vec3_origin, (int)points, 0, DAMAGE_ENERGY, MOD_BFG_EFFECT);
		}
	}

	self.nextthink = level.framenum + 1;
	self.s.frame++;
	if (self.s.frame == 5)
		self.think = G_FreeEdict_savable;
}

REGISTER_SAVABLE_FUNCTION(bfg_explode);

static void bfg_touch(entity &self, entity &other, vector normal, const surface &surf)
{
	if (other == self.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict(self);
		return;
	}
#ifdef SINGLE_PLAYER

	if (self.owner->is_client())
		PlayerNoise(self.owner, self.s.origin, PNOISE_IMPACT);
#endif

	// core explosion - prevents firing it into the wall/floor
	if (other.takedamage)
		T_Damage(other, self, self.owner, self.velocity, self.s.origin, normal, 200, 0, DAMAGE_NONE, MOD_BFG_BLAST);
	T_RadiusDamage(self, self.owner, 200, other, 100, MOD_BFG_BLAST);

	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bfg_.x1b.wav"), 1, ATTN_NORM, 0);
	self.solid = SOLID_NOT;
	self.touch = 0;
	self.s.origin += ((-1 * FRAMETIME) * self.velocity);
	self.velocity = vec3_origin;
	self.s.modelindex = gi.modelindex("sprites/s_bfg3.sp2");
	self.s.frame = 0;
	self.s.sound = SOUND_NONE;
	self.s.effects &= ~EF_ANIM_ALLFAST;
	self.think = bfg_explode_savable;
	self.nextthink = level.framenum + 1;
	self.enemy = other;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BFG_BIGEXPLOSION);
	gi.WritePosition(self.s.origin);
	gi.multicast(self.s.origin, MULTICAST_PVS);
}

REGISTER_SAVABLE_FUNCTION(bfg_touch);

static void bfg_think(entity &self)
{
	entityref	ent;
	entityref	ignore;
	vector	point;
	vector	dir;
	vector	start;
	vector	end;
	trace	tr;
#ifdef SINGLE_PLAYER
	int	dmg;

	if (!deathmatch)
		dmg = 10;
	else
		dmg = 5;
#else
	const int dmg = 5;
#endif

	ent = world;
	while ((ent = findradius(ent, self.s.origin, 256)).has_value())
	{
		if (ent == self)
			continue;

		if (ent == self.owner)
			continue;

		if (!ent->takedamage)
			continue;

		if (!(ent->svflags & SVF_MONSTER) && !ent->is_client()
#ifdef SINGLE_PLAYER
			&& ent->type != ET_MISC_EXPLOBOX
#endif
#ifdef GROUND_ZERO
			&& !(ent.svflags & SVF_DAMAGEABLE)
#endif
			)
			continue;

		if (OnSameTeam(self, ent))
			continue;

		point = ent->absmin + (0.5f * ent->size);

		dir = point - self.s.origin;
		VectorNormalize(dir);

		ignore = self;
		start = self.s.origin;
		end = start + (2048 * dir);
		while (1)
		{
			tr = gi.traceline(start, end, ignore, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

			if (tr.ent.is_world())
				break;

			// hurt it if we can
			if (tr.ent.takedamage && !(tr.ent.flags & FL_IMMUNE_LASER) && (tr.ent != self.owner))
				T_Damage(tr.ent, self, self.owner, dir, tr.endpos, vec3_origin, dmg, 1, DAMAGE_ENERGY, MOD_BFG_LASER);

			// if we hit something that's not a monster or player we're done
			if (!(tr.ent.svflags & SVF_MONSTER) && !tr.ent.is_client()
#ifdef GROUND_ZERO
				&& !(tr.ent.svflags & SVF_DAMAGEABLE)
#endif
				)
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(4);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.normal);
				gi.WriteByte((uint8_t)self.s.skinnum);
				gi.multicast(tr.endpos, MULTICAST_PVS);
				break;
			}

			ignore = tr.ent;
			start = tr.endpos;
		}

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BFG_LASER);
		gi.WritePosition(self.s.origin);
		gi.WritePosition(tr.endpos);
		gi.multicast(self.s.origin, MULTICAST_PHS);
	}

	self.nextthink = level.framenum + 1;
}

REGISTER_SAVABLE_FUNCTION(bfg_think);

void fire_bfg(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius)
{
	entity &bfg = G_Spawn();
	bfg.s.origin = start;
	bfg.s.angles = vectoangles(dir);
	bfg.velocity = dir * speed;
	bfg.movetype = MOVETYPE_FLYMISSILE;
	bfg.clipmask = MASK_SHOT;
	bfg.solid = SOLID_BBOX;
	bfg.s.effects |= EF_BFG | EF_ANIM_ALLFAST;
	bfg.mins = vec3_origin;
	bfg.maxs = vec3_origin;
	bfg.s.modelindex = gi.modelindex("sprites/s_bfg1.sp2");
	bfg.owner = self;
	bfg.touch = bfg_touch_savable;
	bfg.nextthink = level.framenum + BASE_FRAMERATE * 8000 / speed;
	bfg.think = G_FreeEdict_savable;
	bfg.radius_dmg = damage;
	bfg.dmg_radius = damage_radius;
	bfg.type = ET_BFG_BLAST;
	bfg.s.sound = gi.soundindex("weapons/bfg__l1a.wav");

	bfg.think = bfg_think_savable;
	bfg.nextthink = level.framenum + 1;
	bfg.teammaster = bfg;
	bfg.teamchain = world;

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge(self, bfg.s.origin, dir, speed);
#endif

	gi.linkentity(bfg);
}