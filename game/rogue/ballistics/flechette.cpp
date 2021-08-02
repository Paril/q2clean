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
		PlayerNoise(self.owner, self.s.origin, PNOISE_IMPACT);
#endif

	if (other.takedamage)
	{
		T_Damage (other, self, self.owner, self.velocity, self.s.origin, normal,
			self.dmg, self.dmg_radius, DAMAGE_NO_REG_ARMOR, MOD_ETF_RIFLE);
	}
	else
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_FLECHETTE);
		gi.WritePosition (self.s.origin);
		gi.WriteDir (normal * 256);
		gi.multicast (self.s.origin, MULTICAST_PVS);
	}

	G_FreeEdict (self);
}

static REGISTER_SAVABLE_FUNCTION(flechette_touch);

void fire_flechette(entity &self, vector start, vector dir, int32_t damage, int32_t speed, int32_t kick)
{
	VectorNormalize (dir);

	entity &flechette = G_Spawn();
	flechette.s.origin = flechette.s.old_origin = start;
	flechette.s.angles = vectoangles (dir);

	flechette.velocity = dir * speed;
	flechette.movetype = MOVETYPE_FLYMISSILE;
	flechette.clipmask = MASK_SHOT;
	flechette.solid = SOLID_BBOX;
	flechette.s.renderfx = RF_FULLBRIGHT;
	
	flechette.s.modelindex = gi.modelindex ("models/proj/flechette/tris.md2");

	flechette.owner = self;
	flechette.touch = SAVABLE(flechette_touch);
	flechette.nextthink = level.framenum + ((8000 / speed) * BASE_FRAMERATE);
	flechette.think = SAVABLE(G_FreeEdict);
	flechette.dmg = damage;
	flechette.dmg_radius = (float)kick;

	gi.linkentity (flechette);
	
#ifdef SINGLE_PLAYER
	if (self.is_client())
		check_dodge (self, flechette.s.origin, dir, speed);
#endif
}
#endif