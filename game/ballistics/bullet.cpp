import "config.h";
import "game/entity.h";
import "lib/math/random.h";
import "game/weaponry.h";
import "game/combat.h";

/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
void fire_lead(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, temp_event te_impact, int32_t hspread, int32_t vspread, means_of_death mod)
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

/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, means_of_death mod)
{
	fire_lead(self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);
}

/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
*/
void fire_shotgun(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, int32_t count, means_of_death mod)
{
	for (int32_t i = 0; i < count; i++)
		fire_lead(self, start, aimdir, damage, kick, TE_SHOTGUN, hspread, vspread, mod);
}