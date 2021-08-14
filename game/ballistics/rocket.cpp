#include "config.h"
#include "lib/gi.h"
#include "game/util.h"
#include "game/weaponry.h"
#include "game/ballistics.h"
#include "lib/math/random.h"
#include "game/combat.h"
#include "game/game.h"
#include "game/entity.h"
#include "game/misc.h"
#include "rocket.h"

constexpr means_of_death MOD_ROCKET_SPLASH { .self_kill_fmt = "{0} blew {2} up.\n", .other_kill_fmt = "{0} almost dodged {3}'s rocket.\n" };
constexpr means_of_death MOD_ROCKET { .other_kill_fmt = "{0} ate {3}'s rocket.\n" };

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
		PlayerNoise(ent.owner, ent.origin, PNOISE_IMPACT);
#endif

	// calculate position for the explosion entity
	origin = ent.origin + (-0.02f * ent.velocity);

	if (other.takedamage)
		T_Damage(other, ent, ent.owner, ent.velocity, ent.origin, normal, ent.dmg, 0, { DAMAGE_NONE }, MOD_ROCKET);
#ifdef SINGLE_PLAYER
	// don't throw any debris in net games
	else if (!deathmatch && !coop)
	{
		if (!(surf.flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66 | SURF_FLOWING)))
		{
			int32_t n = Q_rand() % 5;
			while (n--)
				ThrowDebris(ent, "models/objects/debris2/tris.md2", 2, ent.origin);
		}
	}
#endif

	T_RadiusDamage(ent, ent.owner, (float) ent.radius_dmg, other, ent.dmg_radius, MOD_ROCKET_SPLASH);

	gi.ConstructMessage(svc_temp_entity, ent.waterlevel ? TE_ROCKET_EXPLOSION_WATER : TE_ROCKET_EXPLOSION, origin).multicast(ent.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

REGISTER_STATIC_SAVABLE(rocket_touch);

entity &fire_rocket(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius, int radius_damage)
{
	entity &rocket = G_Spawn();
	rocket.origin = start;
	rocket.angles = vectoangles(dir);
	rocket.velocity = dir * speed;
	rocket.movetype = MOVETYPE_FLYMISSILE;
	rocket.clipmask = MASK_SHOT;
	rocket.solid = SOLID_BBOX;
	rocket.effects |= EF_ROCKET;
	rocket.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
	rocket.owner = self;
	rocket.touch = SAVABLE(rocket_touch);
	rocket.nextthink = level.framenum + seconds(8000 / speed);
	rocket.think = SAVABLE(G_FreeEdict);
	rocket.dmg = damage;
	rocket.radius_dmg = radius_damage;
	rocket.dmg_radius = damage_radius;
	rocket.sound = gi.soundindex("weapons/rockfly.wav");

#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge(self, rocket.origin, dir, speed);
#endif

	gi.linkentity(rocket);
	return rocket;
}
