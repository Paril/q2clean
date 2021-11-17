#include "config.h"

#ifdef SINGLE_PLAYER
#include "lib/gi.h"
#include "lib/math/random.h"
#include "lib/math/vector.h"
#include "game/util.h"
#include "game/weaponry.h"
#include "game/combat.h"
#include "game/entity.h"
#include "game/game.h"
#include "hit.h"

constexpr means_of_death MOD_HIT = { .other_kill_fmt = "{0} was hacked'n'slashed by {3}.\n" };

/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
bool fire_hit(entity &self, vector aim, int32_t damage, int32_t kick)
{
	trace	tr;
	vector	forward, right, up;
	vector	v;
	vector	point;
	float	range;
	vector	dir;

	//see if enemy is in range
	dir = self.enemy->origin - self.origin;
	range = VectorLength(dir);

	if (range > aim.x)
		return false;

	if (aim.y > self.bounds.mins.x && aim.y < self.bounds.maxs.x)
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self.enemy->bounds.maxs.x;
	// this is a side hit so adjust the "right" value out to the edge of their bbox
	else if (aim.y < 0)
		aim.y = self.enemy->bounds.mins.x;
	else
		aim.y = self.enemy->bounds.maxs.x;

	point = self.origin + (range * dir);

	tr = gi.traceline(self.origin, point, self, MASK_SHOT);

	entityref hit_entity = tr.ent;

	if (tr.fraction < 1)
	{
		if (!tr.ent.takedamage)
			return false;
		// if it will hit any client/monster then hit the one we wanted to hit
		if ((tr.ent.svflags & SVF_MONSTER) || (tr.ent.is_client))
			hit_entity = self.enemy;
	}

	AngleVectors(self.angles, &forward, &right, &up);
	point = self.origin + (range * forward) + (aim.y * right) + (aim.z * up);
	dir = point - self.enemy->origin;

	// do the damage
	T_Damage(hit_entity, self, self, dir, point, vec3_origin, damage, kick / 2, { DAMAGE_NO_KNOCKBACK }, MOD_HIT);

	if (!(hit_entity->svflags & SVF_MONSTER) && (!hit_entity->is_client))
		return false;

	// do our special form of knockback here
	v = (self.enemy->absbounds.mins + (0.5f * self.enemy->size)) - point;
	VectorNormalize(v);
	self.enemy->velocity += (kick * v);
	if (self.enemy->velocity.z > 0)
		self.enemy->groundentity = null_entity;
	return true;
}
#endif