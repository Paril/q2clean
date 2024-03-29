#include "config.h"
#include "game/entity.h"
#include "lib/math/random.h"
#include "game/weaponry.h"
#include "game/combat.h"
#include "bullet.h"

/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
inline void fire_lead(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, temp_event te_impact, int32_t hspread, int32_t vspread, means_of_death_ref mod)
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

	tr = gi.traceline(self.origin, start, self, MASK_SHOT);
	if (!(tr.fraction < 1.0f))
	{
		dir = vectoangles(aimdir);

		AngleVectors(dir, &forward, &right, &up);

		r = crandom(hspread);
		u = crandom(vspread);
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
					gi.ConstructMessage(svc_temp_entity, TE_SPLASH, uint8_t { 8 }, tr.endpos, vecdir { tr.normal }, color).multicast(tr.endpos, MULTICAST_PVS);

				// change bullet's course when it enters water
				dir = vectoangles(end - start);
				AngleVectors(dir, &forward, &right, &up);
				r = crandom(hspread * 2);
				u = crandom(vspread * 2);
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
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.normal, damage, kick, { .sparks = TE_BULLET_SPARKS }, mod);
			else if (strncmp(tr.surface.name.data(), "sky", 3) != 0)
			{
				gi.ConstructMessage(svc_temp_entity, te_impact, tr.endpos, vecdir { tr.normal }).multicast(tr.endpos, MULTICAST_PVS);

#ifdef SINGLE_PLAYER
				if (self.is_client)
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

		gi.ConstructMessage(svc_temp_entity, TE_BUBBLETRAIL, water_start, tr.endpos).multicast(pos, MULTICAST_PVS);
	}
}

/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, means_of_death_ref mod)
{
	fire_lead(self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);
}

/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
*/
void fire_shotgun(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, int32_t count, means_of_death_ref mod)
{
	for (int32_t i = 0; i < count; i++)
		fire_lead(self, start, aimdir, damage, kick, TE_SHOTGUN, hspread, vspread, mod);
}