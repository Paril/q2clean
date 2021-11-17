#include "config.h"
#include "game/entity.h"
#include "lib/gi.h"
#include "game/util.h"
#include "game/weaponry.h"
#include "game/ballistics.h"
#include "lib/math/random.h"
#include "game/combat.h"
#include "lib/math/vector.h"
#include "grenade.h"

constexpr spawn_flag GRENADE_IS_HAND = (spawn_flag) 1;
constexpr spawn_flag GRENADE_IS_HELD = (spawn_flag) 2;

constexpr means_of_death MOD_HELD_GRENADE { .self_kill_fmt = "{0} tried to put the pin back in.\n", .other_kill_fmt = "{0} feels {3}'s pain.\n" };
constexpr means_of_death MOD_GRENADE_SPLASH { .self_kill_fmt = "{0} tripped on {1} own grenade.\n", .other_kill_fmt = "{0} was shredded by {3}'s shrapnel.\n" };
constexpr means_of_death MOD_HANDGRENADE_SPLASH { .self_kill_fmt = "{0} tripped on {1} own handgrenade.\n", .other_kill_fmt = "{0} didn't see {3}'s handgrenade.\n" };

constexpr means_of_death MOD_GRENADE { .other_kill_fmt = "{0} was popped by {3}'s grenade.\n" };
constexpr means_of_death MOD_HANDGRENADE { .other_kill_fmt = "{0} caught {3}'s handgrenade.\n" };

void Grenade_Explode(entity &ent)
{
#ifdef SINGLE_PLAYER
	if (ent.owner->is_client)
		PlayerNoise(ent.owner, ent.origin, PNOISE_IMPACT);

#endif
	means_of_death_ref mod = (ent.spawnflags & GRENADE_IS_HELD) ? MOD_HELD_GRENADE :
		(ent.spawnflags & GRENADE_IS_HAND) ? MOD_HANDGRENADE_SPLASH :
		MOD_GRENADE_SPLASH;

	if (ent.enemy.has_value())
	{
		vector v = ent.enemy->origin + ent.enemy->bounds.center();
		v = ent.origin - v;
		float points = ent.dmg - 0.5f * VectorLength(v);
		vector dir = ent.enemy->origin - ent.origin;

		T_Damage(ent.enemy, ent, ent.owner, dir, ent.origin, vec3_origin, (int32_t) points, (int32_t) points, { DAMAGE_RADIUS }, mod);
	}

	T_RadiusDamage(ent, ent.owner, (float) ent.dmg, ent.enemy, ent.dmg_radius, mod);

	vector origin = ent.origin + (-0.02f * ent.velocity);

	constexpr temp_event events[] { TE_GRENADE_EXPLOSION, TE_ROCKET_EXPLOSION, TE_GRENADE_EXPLOSION_WATER, TE_ROCKET_EXPLOSION_WATER };

	gi.ConstructMessage(svc_temp_entity, events[boolbits(ent.waterlevel, ent.groundentity.has_value())], origin).multicast(ent.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

REGISTER_STATIC_SAVABLE(Grenade_Explode);

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
			gi.sound(ent, CHAN_VOICE, gi.soundindex(random() > 0.5f ? "weapons/hgrenb1a.wav" : "weapons/hgrenb2a.wav"));
		else
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/grenlb1b.wav"));

		return;
	}

	ent.enemy = other;
	Grenade_Explode(ent);
}

REGISTER_STATIC_SAVABLE(Grenade_Touch);

entity_type ET_GRENADE("grenade");

void fire_grenade(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, gtimef timer, float damage_radius)
{
	vector	dir;
	vector	forward, right, up;
	float   scale;

	dir = vectoangles(aimdir);
	AngleVectors(dir, &forward, &right, &up);

	entity &grenade = G_Spawn();
	grenade.origin = start;
	grenade.velocity = aimdir * speed;
	scale = random(190.f, 210.f);
	grenade.velocity += (scale * up);
	scale = crandom(10.f);
	grenade.velocity += (scale * right);
	grenade.avelocity = { 300, 300, 300 };
	grenade.movetype = MOVETYPE_BOUNCE;
	grenade.clipmask = MASK_SHOT;
	grenade.solid = SOLID_BBOX;
	grenade.effects |= EF_GRENADE;
	grenade.modelindex = gi.modelindex("models/objects/grenade/tris.md2");
	grenade.owner = self;
	grenade.touch = SAVABLE(Grenade_Touch);
	grenade.nextthink = duration_cast<gtime>(level.time + timer);
	grenade.think = SAVABLE(Grenade_Explode);
	grenade.dmg = damage;
	grenade.dmg_radius = damage_radius;
	grenade.type = ET_GRENADE;

	gi.linkentity(grenade);
}

void fire_grenade2(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, gtimef timer, float damage_radius, bool held)
{
	vector	dir;
	vector	forward, right, up;
	float   scale;

	dir = vectoangles(aimdir);
	AngleVectors(dir, &forward, &right, &up);

	entity &grenade = G_Spawn();
	grenade.origin = start;
	grenade.velocity = aimdir * speed;
	scale = random(190.f, 210.f);
	grenade.velocity += (scale * up);
	scale = crandom(10.f);
	grenade.velocity += (scale * right);
	grenade.avelocity = { 300, 300, 300 };
	grenade.movetype = MOVETYPE_BOUNCE;
	grenade.clipmask = MASK_SHOT;
	grenade.solid = SOLID_BBOX;
	grenade.effects |= EF_GRENADE;
	grenade.modelindex = gi.modelindex("models/objects/grenade2/tris.md2");
	grenade.owner = self;
	grenade.touch = SAVABLE(Grenade_Touch);
	grenade.nextthink = duration_cast<gtime>(level.time + timer);
	grenade.think = SAVABLE(Grenade_Explode);
	grenade.dmg = damage;
	grenade.dmg_radius = damage_radius;
	grenade.type = ET_GRENADE;
	grenade.spawnflags = GRENADE_IS_HAND;
	if (held)
		grenade.spawnflags |= GRENADE_IS_HELD;
	grenade.sound = gi.soundindex("weapons/hgrenc1b.wav");

	if (timer <= gtimef::zero())
		Grenade_Explode(grenade);
	else
	{
		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/hgrent1a.wav"));
		gi.linkentity(grenade);
	}
}
