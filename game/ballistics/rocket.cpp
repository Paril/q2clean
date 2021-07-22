import "config.h";
import "lib/gi.h";
import "game/util.h";
import "game/weaponry.h";
import "game/ballistics.h";
import "lib/math/random.h";
import "game/combat.h";
import "game/game.h";
import "game/entity.h";
import "game/misc.h";

void rocket_touch(entity &ent, entity &other, vector normal, const surface &surf)
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

	T_RadiusDamage(ent, ent.owner, (float) ent.radius_dmg, other, ent.dmg_radius, MOD_R_SPLASH);

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
