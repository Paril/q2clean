#include "config.h"

#ifdef GROUND_ZERO
#include "game/combat.h"
#include "game/entity.h"
#include "game/util.h"
#ifdef SINGLE_PLAYER
#include "game/weaponry.h"
#include "game/ballistics.h"
#endif
#include "flechette.h"

constexpr means_of_death MOD_ETF_RIFLE { .other_kill_fmt = "{0} was perforated by {3}.\n" };

static void flechette_touch(entity &self, entity &other, vector normal, const surface &surf)
{
	if (other == self.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict (self);
		return;
	}
#if defined(SINGLE_PLAYER)

	if (self.is_client())
		PlayerNoise(self.owner, self.origin, PNOISE_IMPACT);
#endif

	if (other.takedamage)
		T_Damage (other, self, self.owner, self.velocity, self.origin, normal,
			self.dmg, self.dmg_radius, { DAMAGE_NO_REG_ARMOR }, MOD_ETF_RIFLE);
	else
		gi.ConstructMessage(svc_temp_entity, TE_FLECHETTE, self.origin, vecdir { normal * 256 }).multicast(self.origin, MULTICAST_PVS);

	G_FreeEdict (self);
}

REGISTER_STATIC_SAVABLE(flechette_touch);

void fire_flechette(entity &self, vector start, vector dir, int32_t damage, int32_t speed, int32_t kick)
{
	VectorNormalize (dir);

	entity &flechette = G_Spawn();
	flechette.origin = flechette.old_origin = start;
	flechette.angles = vectoangles (dir);

	flechette.velocity = dir * speed;
	flechette.movetype = MOVETYPE_FLYMISSILE;
	flechette.clipmask = MASK_SHOT;
	flechette.solid = SOLID_BBOX;
	flechette.renderfx = RF_FULLBRIGHT;
	
	flechette.modelindex = gi.modelindex ("models/proj/flechette/tris.md2");

	flechette.owner = self;
	flechette.touch = SAVABLE(flechette_touch);
	flechette.nextthink = level.framenum + ((8000 / speed) * BASE_FRAMERATE);
	flechette.think = SAVABLE(G_FreeEdict);
	flechette.dmg = damage;
	flechette.dmg_radius = (float)kick;

	gi.linkentity (flechette);
	
#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge (self, flechette.origin, dir, speed);
#endif
}
#endif