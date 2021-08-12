#include "config.h"

#ifdef SINGLE_PLAYER
#include "game/entity.h"
#include "game/game.h"
#include "game/ai.h"
#include "game/move.h"
#include "game/util.h"
#include "game/spawn.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "ai.h"

#ifdef ROGUE_AI
#include "game/func.h"
#include "game/monster.h"

#ifdef GROUND_ZERO
#include "game/rogue/ballistics/tesla.h"
#endif

//===============================
// BLOCKED Logic
//===============================
#if 0
bool parasite_drain_attack_ok(vector start, vector end);
#endif

// blocked_checkshot
//	shot_chance: 0-1, chance they'll take the shot if it's clear.
bool blocked_checkshot(entity &self, float shot_chance)
{
	if (!self.enemy.has_value())
		return false;

	if (!visible(self, self.enemy))
		return false;

#ifdef GROUND_ZERO
	// always shoot at teslas
	if (self.enemy->type == ET_TESLA)
	{
		// turn on AI_BLOCKED to let the monster know the attack is being called
		// by the blocked functions...
		self.monsterinfo.aiflags |= AI_BLOCKED;

		if (self.monsterinfo.attack)
			self.monsterinfo.attack(self);

		self.monsterinfo.aiflags &= ~AI_BLOCKED;
		return true;
	}
#endif

	// blocked checkshot is only against players. this will
	// filter out player sounds and other shit they should
	// not be firing at.
	if (!self.enemy->is_client())
		return false;

	if (random() < shot_chance)
	{
		// turn on AI_BLOCKED to let the monster know the attack is being called
		// by the blocked functions...
		self.monsterinfo.aiflags |= AI_BLOCKED;

		if (self.monsterinfo.attack)
			self.monsterinfo.attack(self);

		self.monsterinfo.aiflags &= ~AI_BLOCKED;
		return true;
	}

#if 0
	// PMM - special handling for the parasite
	if (self.classname == "monster_parasite")
	{
		vector	f, r, start;
		AngleVectors (self.angles, &f, &r, 0);
		start = G_ProjectSource (self.origin, '24 0 6', f, r);

		vector end = self.enemy.origin;
		if (!parasite_drain_attack_ok(start, end))
		{
			end[2] = self.enemy.origin[2] + self.enemy.maxs[2] - 8;
			if (!parasite_drain_attack_ok(start, end))
			{
				end[2] = self.enemy.origin[2] + self.enemy.mins[2] + 8;
				if (!parasite_drain_attack_ok(start, end))
					return false;
			}
		}
		end = self.enemy.origin;

		trace_t	tr;
		gi.traceline (&tr, start, end, self, MASK_SHOT);

		if (tr.ent != self.enemy)
		{
			self.monsterinfo.aiflags |= AI_BLOCKED;
			
			if (self.monsterinfo.attack)
				self.monsterinfo.attack(self);
			
			self.monsterinfo.aiflags &= ~AI_BLOCKED;
			return true;
		}
	}
#endif

	return false;
}

// blocked_checkplat
//	dist: how far they are trying to walk.
bool blocked_checkplat(entity &self, float dist)
{
	if (!self.enemy.has_value())
		return false;

	int player_position;

	// check player's relative altitude
	if (self.enemy->absbounds.mins[2] >= self.absbounds.maxs[2])
		player_position = 1;
	else if (self.enemy->absbounds.maxs[2] <= self.absbounds.mins[2])
		player_position = -1;
	else // if we're close to the same position, don't bother trying plats.
		return false;

	entityref plat;

	// see if we're already standing on a plat.
	if (self.groundentity.has_value())
		if (self.groundentity->type == ET_FUNC_PLAT)
			plat = self.groundentity;

	// if we're not, check to see if we'll step onto one with this move
	if (!plat.has_value())
	{
		vector forward;
		AngleVectors (self.angles, &forward, nullptr, nullptr);
		vector pt1 = self.origin + (forward * dist);
		vector pt2 = pt1;
		pt2[2] -= 384;

		trace trace = gi.traceline(pt1, pt2, self, MASK_MONSTERSOLID);

		if (trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
			if (trace.ent.type == ET_FUNC_PLAT)
				plat = trace.ent;
	}

	if (!plat.has_value() || !plat->use)
		return false;

	// if we've found a plat, trigger it.
	if (player_position == 1)
	{
		if ((self.groundentity == plat && plat->moveinfo.state == STATE_BOTTOM) ||
			(self.groundentity != plat && plat->moveinfo.state == STATE_TOP))
		{
			plat->use (plat, self, self);
			return true;			
		}
	}
	else if(player_position == -1)
	{
		if ((self.groundentity == plat && plat->moveinfo.state == STATE_TOP) ||
			(self.groundentity != plat && plat->moveinfo.state == STATE_BOTTOM))
		{
			plat->use (plat, self, self);
			return true;
		}
	}

	return false;
}

static bool face_wall(entity &self)
{
	vector forward;
	AngleVectors (self.angles, &forward, nullptr, nullptr);

	vector pt = self.origin + (forward * 64);

	trace tr = gi.traceline(self.origin, pt, self, MASK_MONSTERSOLID);

	if (tr.fraction < 1 && !tr.allsolid && !tr.startsolid)
	{
		vector ang = vectoangles(tr.normal);

		self.ideal_yaw = ang[YAW] + 180.f;
		if (self.ideal_yaw > 360)
			self.ideal_yaw -= 360;

		M_ChangeYaw(self);
		return true;
	}

	return false;
}

// blocked_checkjump
//	dist: how far they are trying to walk.
//  maxDown/maxUp: how far they'll ok a jump for. set to 0 to disable that direction.
bool blocked_checkjump(entity &self, float, float max_down, float max_up)
{
	if (!self.enemy.has_value())
		return false;

	vector forward, up;
	AngleVectors (self.angles, &forward, nullptr, &up);

	int playerPosition;

	if (self.enemy->absbounds.mins[2] > (self.absbounds.mins[2] + 16))
		playerPosition = 1;
	else if (self.enemy->absbounds.mins[2] < (self.absbounds.mins[2] - 16))
		playerPosition = -1;
	else
		return false;

	if (playerPosition == -1 && max_down)
	{
		// check to make sure we can even get to the spot we're going to "fall" from
		vector pt1 = self.origin + (forward * 48);
		trace trace = gi.trace(self.origin, self.bounds, pt1, self, MASK_MONSTERSOLID);

		if(trace.fraction < 1)
			return false;

		vector pt2 = pt1;
		pt2[2] = self.bounds.mins[2] - max_down - 1;

		trace = gi.traceline(pt1, pt2, self, MASK_MONSTERSOLID | MASK_WATER);

		if(trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			if((self.absbounds.mins[2] - trace.endpos[2]) >= 24 && trace.contents & MASK_SOLID)
			{
				if ((self.enemy->absbounds.mins[2] - trace.endpos[2]) > 32)
					return false;

				if(trace.normal[2] < 0.9)
					return false;

				return true;
			}
		}
	}
	else if (playerPosition == 1 && max_up)
	{
		vector pt1 = self.origin + (forward * 48);
		vector pt2 = pt1;
		pt1[2] = self.absbounds.maxs[2] + max_up;

		trace trace = gi.traceline(pt1, pt2, self, MASK_MONSTERSOLID | MASK_WATER);

		if(trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			if((trace.endpos[2] - self.absbounds.mins[2]) <= max_up && trace.contents & MASK_SOLID)
			{
				face_wall(self);
				return true;
			}
		}
	}

	return false;
}

// *************************
// HINT PATHS
// *************************

constexpr spawn_flag HINT_ENDPOINT	= (spawn_flag) 0x0001;
constexpr uint32_t MAX_HINT_CHAINS	= 100;

static array<entityref, MAX_HINT_CHAINS>	hint_path_start;
static bool hint_paths_present;
static uint32_t num_hint_paths;

// =============
// hintpath_go - starts a monster (self) moving towards the hintpath (point)
//		disables all contrary AI flags.
// =============
static void hintpath_go(entity &self, entity &point)
{
	self.ideal_yaw = vectoyaw(point.origin - self.origin);
	self.goalentity = self.movetarget = point;
	self.monsterinfo.pause_framenum = 0;
	self.monsterinfo.aiflags |= AI_HINT_PATH;
	self.monsterinfo.aiflags &= ~(AI_SOUND_TARGET | AI_PURSUIT_LAST_SEEN | AI_PURSUE_NEXT | AI_PURSUE_TEMP);
	// run for it
	self.monsterinfo.search_framenum = level.framenum;
	self.monsterinfo.run (self);
}

bool has_valid_enemy(entity &self)
{
	return self.enemy.has_value() && self.enemy->inuse && self.enemy->health;
}

// =============
// hintpath_stop - bails a monster out of following hint paths
// =============
void hintpath_stop(entity &self)
{
	self.goalentity = 0;
	self.movetarget = 0;
	self.monsterinfo.last_hint_framenum = level.framenum;
	self.monsterinfo.goal_hint = 0;
	self.monsterinfo.aiflags &= ~AI_HINT_PATH;

	if (has_valid_enemy(self))
	{
		// if we can see our target, go nuts
		if (visible(self, self.enemy))
		{
			FoundTarget (self);
			return;
		}
		// otherwise, keep chasing
		HuntTarget (self);
		return;
	}

	// if our enemy is no longer valid, forget about our enemy and go into stand
	self.enemy = 0;
	// we need the pausetime otherwise the stand code
	// will just revert to walking with no target and
	// the monsters will wonder around aimlessly trying
	// to hunt the world entity
	self.monsterinfo.pause_framenum = INT_MAX;
	self.monsterinfo.stand (self);
}

// temp
static entity_type ET_MONSTER_TURRET("temp");

// =============
// monsterlost_checkhint - the monster (self) will check around for valid hintpaths.
//		a valid hintpath is one where the two endpoints can see both the monster
//		and the monster's enemy. if only one person is visible from the endpoints,
//		it will not go for it.
// =============
bool monsterlost_checkhint(entity &self)
{
	entityref	e, monster_pathchain, target_pathchain, checkpoint;
	entityref	closest;
	float		closest_range = 1000000;
	entityref	start, destination;
	uint32_t		count1=0, count2=0, count3=0, count4=0, count5=0;
	float		r;
	uint32_t		i;
	array<bool, MAX_HINT_CHAINS> hint_path_represented;

	// if there are no hint paths on this map, exit immediately.
	if(!hint_paths_present)
		return false;

	if(!self.enemy.has_value())
		return false;

	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		return false;
	
#ifdef GROUND_ZERO
	if (self.type == ET_MONSTER_TURRET)
		return false;
#endif

	monster_pathchain = 0;

	// find all the hint_paths.
	// FIXME - can we not do this every time?
	for (i=0; i < num_hint_paths; i++)
	{
		e = hint_path_start[i];
		while(e.has_value())
		{
			count1++;
			if (e->monster_hint_chain.has_value())
				e->monster_hint_chain = 0;

			if (monster_pathchain.has_value())
			{
				checkpoint->monster_hint_chain = e;
				checkpoint = e;
			}
			else
			{
				monster_pathchain = e;
				checkpoint = e;
			}

			e = e->hint_chain;
		}
	}
	
	// filter them by distance and visibility to the monster
	e = monster_pathchain;
	checkpoint = 0;
	while (e.has_value())
	{
		r = VectorDistance (self.origin, e->origin);

		if (r > 512)
		{
			count2++;
			if (checkpoint.has_value())
			{
				checkpoint->monster_hint_chain = e->monster_hint_chain;
				e->monster_hint_chain = 0;
				e = checkpoint->monster_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->monster_hint_chain;
				checkpoint->monster_hint_chain = 0;
				// and clear it again
				checkpoint = 0;
				// since we have yet to find a valid one (or else checkpoint would be set) move the
				// start of monster_pathchain
				monster_pathchain = e;
				continue;
			}
		}
		if (!visible(self, e))
		{
			count4++;
			if (checkpoint.has_value())
			{
				checkpoint->monster_hint_chain = e->monster_hint_chain;
				e->monster_hint_chain = 0;
				e = checkpoint->monster_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->monster_hint_chain;
				checkpoint->monster_hint_chain = 0;
				// and clear it again
				checkpoint = 0;
				// since we have yet to find a valid one (or else checkpoint would be set) move the
				// start of monster_pathchain
				monster_pathchain = e;
				continue;
			}
		}

		count5++;
		checkpoint = e;
		e = e->monster_hint_chain;
	}

	// at this point, we have a list of all of the eligible hint nodes for the monster
	// we now take them, figure out what hint chains they're on, and traverse down those chains,
	// seeing whether any can see the player
	//
	// first, we figure out which hint chains we have represented in monster_pathchain
	if (count5 == 0)
		return false;

	for (i=0; i < num_hint_paths; i++)
		hint_path_represented[i] = false;

	e = monster_pathchain;
	checkpoint = 0;
	while (e.has_value())
	{
		if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
			return false;

		hint_path_represented[e->hint_chain_id] = true;
		e = e->monster_hint_chain;
	}

	count1 = 0;
	count2 = 0;
	count3 = 0;
	count4 = 0;
	count5 = 0;

	// now, build the target_pathchain which contains all of the hint_path nodes we need to check for
	// validity (within range, visibility)
	target_pathchain = 0;
	checkpoint = 0;
	for (i=0; i < num_hint_paths; i++)
	{
		// if this hint chain is represented in the monster_hint_chain, add all of it's nodes to the target_pathchain
		// for validity checking
		if (hint_path_represented[i])
		{
			e = hint_path_start[i];
			while (e.has_value())
			{
				if (target_pathchain.has_value())
				{
					checkpoint->target_hint_chain = e;
					checkpoint = e;
				}
				else
				{
					target_pathchain = e;
					checkpoint = e;
				}
				e = e->hint_chain;
			}
		}
	}

	// target_pathchain is a list of all of the hint_path nodes we need to check for validity relative to the target
	e = target_pathchain;
	checkpoint = 0;
	while (e.has_value())
	{
		r = VectorDistance (self.enemy->origin, e->origin);

		if (r > 512)
		{
			count2++;

			if (checkpoint.has_value())
			{
				checkpoint->target_hint_chain = e->target_hint_chain;
				e->target_hint_chain = 0;
				e = checkpoint->target_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->target_hint_chain;
				checkpoint->target_hint_chain = 0;
				// and clear it again
				checkpoint = 0;
				target_pathchain = e;
				continue;
			}
		}
		if (!visible(self.enemy, e))
		{
			count4++;

			if (checkpoint.has_value())
			{
				checkpoint->target_hint_chain = e->target_hint_chain;
				e->target_hint_chain = 0;
				e = checkpoint->target_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->target_hint_chain;
				checkpoint->target_hint_chain = 0;
				// and clear it again
				checkpoint = 0;
				target_pathchain = e;
				continue;
			}
		}

		count5++;
		checkpoint = e;
		e = e->target_hint_chain;
	}
	
	// at this point we should have:
	// monster_pathchain - a list of "monster valid" hint_path nodes linked together by monster_hint_chain
	// target_pathcain - a list of "target valid" hint_path nodes linked together by target_hint_chain.  these
	//                   are filtered such that only nodes which are on the same chain as "monster valid" nodes
	//
	// Now, we figure out which "monster valid" node we want to use
	// 
	// To do this, we first off make sure we have some target nodes.  If we don't, there are no valid hint_path nodes
	// for us to take
	//
	// If we have some, we filter all of our "monster valid" nodes by which ones have "target valid" nodes on them
	//
	// Once this filter is finished, we select the closest "monster valid" node, and go to it.

	if (count5 == 0)
		return false;

	// reuse the hint_chain_represented array, this time to see which chains are represented by the target
	for (i=0; i < num_hint_paths; i++)
		hint_path_represented[i] = false;

	e = target_pathchain;
	checkpoint = 0;
	while (e.has_value())
	{
		if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
			return false;

		hint_path_represented[e->hint_chain_id] = true;
		e = e->target_hint_chain;
	}
	
	// traverse the monster_pathchain - if the hint_node isn't represented in the "target valid" chain list, 
	// remove it
	// if it is on the list, check it for range from the monster.  If the range is the closest, keep it
	//
	closest = 0;
	e = monster_pathchain;
	while (e.has_value())
	{
		if (!(hint_path_represented[e->hint_chain_id]))
		{
			checkpoint = e->monster_hint_chain;
			e->monster_hint_chain = 0;
			e = checkpoint;
			continue;
		}
		r = VectorDistance(self.origin, e->origin);
		if (r < closest_range)
			closest = e;
		e = e->monster_hint_chain;
	}

	if (!closest.has_value())
		return false;

	start = closest;
	// now we know which one is the closest to the monster .. this is the one the monster will go to
	// we need to finally determine what the DESTINATION node is for the monster .. walk down the hint_chain,
	// and find the closest one to the player

	closest = 0;
	closest_range = 10000000.f;
	e = target_pathchain;
	while (e.has_value())
	{
		if (start->hint_chain_id == e->hint_chain_id)
		{
			r = VectorDistance(self.origin, e->origin);
			if (r < closest_range)
				closest = e;
		}
		e = e->target_hint_chain;
	}

	if (!closest.has_value())
		return false;
	
	destination = closest;

	self.monsterinfo.goal_hint = destination;
	hintpath_go(self, start);
	return true;
}

//
// Path code
//

// =============
// hint_path_touch - someone's touched the hint_path
// =============
static void hint_path_touch(entity &self, entity &other, vector, const surface &)
{
	// make sure we're the target of it's obsession
	if (other.movetarget != self)
		return;

	entityref goal = other.monsterinfo.goal_hint;
	
	// if the monster is where he wants to be
	if (goal == self)
	{
		hintpath_stop (other);
		return;
	}

	// if we aren't, figure out which way we want to go
	entityref e = hint_path_start[self.hint_chain_id], next = 0;
	bool goalFound = false;

	while (e.has_value())
	{
		// if we get up to ourselves on the hint chain, we're going down it
		if (e == self)
		{
			next = e->hint_chain;
			break;
		}

		if (e == goal)
			goalFound = true;

		// if we get to where the next link on the chain is this hint_path and have found the goal on the way
		// we're going upstream, so remember who the previous link is
		if ((e->hint_chain == self) && goalFound)
		{
			next = e;
			break;
		}

		e = e->hint_chain;
	}

	// if we couldn't find it, have the monster go back to normal hunting.
	if (!next.has_value())
	{
		hintpath_stop(other);
		return;
	}

	hintpath_go(other, next);

	// have the monster freeze if the hint path we just touched has a wait time
	// on it, for example, when riding a plat.
	if (self.wait)
		other.nextthink = level.framenum + (int)(self.wait * BASE_FRAMERATE);
}

REGISTER_STATIC_SAVABLE(hint_path_touch);

/*QUAKED hint_path (.5 .3 0) (-8 -8 -8) (8 8 8) END
Target: next hint path

END - set this flag on the endpoints of each hintpath.

"wait" - set this if you want the monster to freeze when they touch this hintpath
*/
static void SP_hint_path(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self.targetname && !self.target)
	{
		gi.dprintfmt("{}: is unlinked\n", self);
		G_FreeEdict (self);
		return;
	}

	self.solid = SOLID_TRIGGER;
	self.touch = SAVABLE(hint_path_touch);
	self.bounds = bbox::sized(8.f);
	self.svflags |= SVF_NOCLIENT;
	gi.linkentity (self);
}

static REGISTER_ENTITY(HINT_PATH, hint_path);

// ============
// InitHintPaths - Called by InitGame (g_save) to enable quick exits if valid
// ============
void InitHintPaths()
{
	entityref	e, current;
	uint32_t	i, count2;
	bool	errors = false;

	hint_paths_present = false;
	
	// check all the hint_paths.
	e = G_FindEquals<&entity::type>(world, ET_HINT_PATH);

	if (e.has_value())
		hint_paths_present = true;
	else
		return;

	hint_path_start = {};
	num_hint_paths = 0;

	while (e.has_value())
	{
		if (e->spawnflags & HINT_ENDPOINT)
		{
			if (e->target) // start point
			{
				if (e->targetname) // this is a bad end, ignore it
				{
					gi.dprintfmt("{}: marked as endpoint with both target ({}) and targetname ({})\n",
						e, e->target, e->targetname);
					errors = true;
				}
				else
				{
					if (num_hint_paths >= MAX_HINT_CHAINS)
						break;

					hint_path_start[num_hint_paths++] = e;
				}
			}
		}
		e = G_FindEquals<&entity::type>(e, ET_HINT_PATH);
	}

	for (i=0; i< num_hint_paths; i++)
	{
		count2 = 1;
		current = hint_path_start[i];
		current->hint_chain_id = i;

		e = G_FindEquals<&entity::targetname>(world, current->target);

		if (G_FindEquals<&entity::targetname>(e, current->target).has_value())
		{
			gi.dprintfmt("{}: fork detected for chain {}, target {}\n", 
				current, num_hint_paths, current->target);
			hint_path_start[i]->hint_chain = 0;
			count2 = 0;
			errors = true;
			continue;
		}

		while (e.has_value())
		{
			if (e->hint_chain.has_value())
			{
				gi.dprintfmt("{}: circular detected for chain {}, targetname {}\n", 
					e, num_hint_paths, e->targetname);
				hint_path_start[i]->hint_chain = 0;
				count2 = 0;
				errors = true;
				break;
			}
			count2++;
			current->hint_chain = e;
			current = e;
			current->hint_chain_id = i;

			if (!current->target)
				break;

			e = G_FindEquals<&entity::targetname>(world, current->target);
			if (G_FindEquals<&entity::targetname>(e, current->target).has_value())
			{
				gi.dprintfmt("{}: fork detected for chain {}, target {}\n", 
					current, num_hint_paths, current->target);
				hint_path_start[i]->hint_chain = 0;
				count2 = 0;
				break;
			}
		}
	}
}

// *****************************
//	MISCELLANEOUS STUFF
// *****************************

// PMM - inback
// use to see if opponent is behind you (not to side)
// if it looks a lot like infront, well, there's a reason
bool inback(entity &self, entity &other)
{
	vector forward;
	AngleVectors (self.angles, &forward, nullptr, nullptr);
	vector vec = other.origin - self.origin;
	VectorNormalize (vec);
	return (vec * forward) < -0.3;
}

//
// Monster "Bad" Areas
// 
static void badarea_touch(entity &, entity &, vector, const surface &)
{
}

REGISTER_STATIC_SAVABLE(badarea_touch);

entity_type ET_BAD_AREA("bad_area");

entity &SpawnBadArea(vector cmins, vector cmaxs, gtime lifespan_frames, entityref cowner)
{
	vector corigin = (cmins + cmaxs) * 0.5f;

	cmaxs -= corigin;
	cmins -= corigin;

	entity &badarea = G_Spawn();
	badarea.origin = corigin;
	badarea.bounds = { .mins = cmins, .maxs = cmaxs };
	badarea.movetype = MOVETYPE_NONE;
	badarea.solid = SOLID_TRIGGER;
	badarea.type = ET_BAD_AREA;
	badarea.touch = SAVABLE(badarea_touch);
	gi.linkentity (badarea);

	if (lifespan_frames)
	{
		badarea.think = SAVABLE(G_FreeEdict);
		badarea.nextthink = level.framenum + lifespan_frames;
	}

	badarea.owner = cowner;

	return badarea;
}

// CheckForBadArea
//		This is a customized version of G_TouchTriggers that will check
//		for bad area triggers and return them if they're touched.
entityref CheckForBadArea(entity &ent)
{
	dynarray<entityref> entities = gi.BoxEdicts(ent.bounds.offsetted(ent.origin), AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (auto hit : entities)
	{
		if (!hit->inuse)
			continue;
		if (hit->touch == badarea_touch)
			return hit;
	}

	return null_entity;
}

// predictive calculator
// target is who you want to shoot
// start is where the shot comes from
// bolt_speed is how fast the shot is
// eye_height is a boolean to say whether or not to adjust to targets eye_height
// offset is how much time to miss by
// aimdir is the resulting aim direction (pass in NULL if you don't want it)
// aimpoint is the resulting aimpoint (pass in NULL if don't want it)
void PredictAim(entityref target, vector start, float bolt_speed, bool eye_height, float offset, vector *aimdir, vector *aimpoint)
{
	if (!target.has_value() || !target->inuse)
	{
		if (aimdir)
			*aimdir = vec3_origin;
		return;
	}

	vector dir = target->origin - start;

	if (eye_height)
		dir[2] += target->viewheight;

	float time = VectorLength(dir) / bolt_speed;

	vector vec = target->origin + (target->velocity * (time - offset));

	if (eye_height)
		vec[2] += target->viewheight;

	if (aimdir)
	{
		*aimdir = vec - start;
		VectorNormalize (*aimdir);
	}
	
	if (aimpoint)
		*aimpoint = vec;
}

bool below(entity &self, entity &other)
{
	vector vec = other.origin - self.origin;
	VectorNormalize (vec);
	return (vec * MOVEDIR_DOWN) > 0.95;  // 18 degree arc below
}

void monster_done_dodge(entity &self)
{
	self.monsterinfo.aiflags &= ~AI_DODGING;
}

//
// New dodge code
//
void M_MonsterDodge(entity &self, entity &attacker, float eta, trace &tr)
{
	// this needs to be here since this can be called after the monster has "died"
	if (self.health < 1)
		return;

	bool ducker = (self.monsterinfo.duck && self.monsterinfo.unduck);
	bool dodger = (self.monsterinfo.sidestep && !(self.monsterinfo.aiflags & AI_STAND_GROUND));

	if (!ducker && !dodger)
		return;

	if (!self.enemy.has_value())
	{
		self.enemy = attacker;
		FoundTarget (self);
	}

	// PMM - don't bother if it's going to hit anyway; fix for weird in-your-face etas (I was
	// seeing numbers like 13 and 14)
	if (eta < 0.1 || eta > 5)
		return;

	// skill level determination..
	if (random() > (0.25f * (skill + 1)))
		return;

	float height;

	if (ducker)
	{
		height = self.absbounds.maxs[2] - 32 - 1;  // the -1 is because the absmax is s.origin + maxs + 1

		// FIXME, make smarter
		// if we only duck, and ducking won't help or we're already ducking, do nothing
		//
		// need to add monsterinfo.abort_duck() and monsterinfo.next_duck_time
		if (!dodger && (tr.endpos[2] <= height || (self.monsterinfo.aiflags & AI_DUCKED)))
			return;
	}
	else
		height = self.absbounds.maxs[2];

	if (dodger)
	{
		// if we're already dodging, just finish the sequence, i.e. don't do anything else
		if (self.monsterinfo.aiflags & AI_DODGING)
			return;

		// if we're ducking already, or the shot is at our knees
		if (tr.endpos[2] <= height || (self.monsterinfo.aiflags & AI_DUCKED))
		{
			vector right;
			AngleVectors (self.angles, nullptr, &right, nullptr);

			vector diff = tr.endpos - self.origin;

			self.monsterinfo.lefty = (right * diff) >= 0;
	
			// if we are currently ducked, unduck

			if (ducker && (self.monsterinfo.aiflags & AI_DUCKED))
				self.monsterinfo.unduck(self);

			self.monsterinfo.aiflags |= AI_DODGING;
			self.monsterinfo.attack_state = AS_SLIDING;

			// call the monster specific code here
			self.monsterinfo.sidestep (self);
			return;
		}
	}

	if (ducker)
	{
		if (self.monsterinfo.next_duck_framenum > level.framenum)
			return;

		monster_done_dodge (self);

		// set this prematurely; it doesn't hurt, and prevents extra iterations
		self.monsterinfo.aiflags |= AI_DUCKED;
		self.monsterinfo.duck (self, eta);
	}
}

REGISTER_SAVABLE(M_MonsterDodge);

void monster_duck_down(entity &self)
{
	self.monsterinfo.aiflags |= AI_DUCKED;
	self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	self.bounds.maxs[2] = self.monsterinfo.base_height - 32;
	self.takedamage = true;
	if (self.monsterinfo.duck_wait_framenum < level.framenum)
		self.monsterinfo.duck_wait_framenum = level.framenum + (1 * BASE_FRAMERATE);
	gi.linkentity (self);
}

void monster_duck_hold(entity &self)
{
	if (level.framenum >= self.monsterinfo.duck_wait_framenum)
		self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self.monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void monster_duck_up(entity &self)
{
	self.monsterinfo.aiflags &= ~AI_DUCKED;
	self.bounds.maxs[2] = self.monsterinfo.base_height;
	self.takedamage = true;
	self.monsterinfo.next_duck_framenum = level.framenum + (int)(DUCK_INTERVAL * BASE_FRAMERATE);
	gi.linkentity (self);
}

REGISTER_SAVABLE(monster_duck_up);

//*******************
// JUMPING AIDS
//*******************

void monster_jump_start(entity &self)
{
	self.timestamp = level.framenum;
}

bool monster_jump_finished(entity &self)
{
	return (level.framenum - self.timestamp) > (3 * BASE_FRAMERATE);
}

// MOVE STUFF
bool IsBadAhead(entity &self, entity &bad, vector move)
{
	vector dir = bad.origin - self.origin;
	VectorNormalize (dir);
	vector forward;
	AngleVectors (self.angles, &forward, nullptr, nullptr);
	float dp_bad = forward * dir;

	VectorNormalize (move);
	AngleVectors (self.angles, &forward, nullptr, nullptr);
	float dp_move = forward * move;

	return (dp_bad < 0 && dp_move < 0) || (dp_bad > 0 && dp_move > 0);
}
#endif

#ifdef GROUND_ZERO
#include "game/rogue/ballistics/tesla.h"

#ifdef ROGUE_AI
bool MarkTeslaArea(entity &tesla)
{
	entityref area;
	// make sure this tesla doesn't have a bad area around it already...
	entityref e = tesla.teamchain;
	entityref tail = tesla;

	while (e.has_value())
	{
		tail = tail->teamchain;

		if (e->type == ET_BAD_AREA)
			return false;

		e = e->teamchain;
	}

	// see if we can grab the trigger directly
	if (tesla.teamchain.has_value() && tesla.teamchain->inuse)
	{
		entity &trigger = tesla.teamchain;

		if (tesla.air_finished_framenum)
			area = SpawnBadArea (trigger.absbounds.mins, trigger.absbounds.maxs, tesla.air_finished_framenum, tesla);
		else
			area = SpawnBadArea (trigger.absbounds.mins, trigger.absbounds.maxs, tesla.nextthink, tesla);
	}
	// otherwise we just guess at how long it'll last.
	else
	{
		vector cmins { -TESLA_DAMAGE_RADIUS, -TESLA_DAMAGE_RADIUS, tesla.bounds.mins[2] };
		vector cmaxs { TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS };

		area = SpawnBadArea(cmins, cmaxs, 30 * BASE_FRAMERATE, tesla);
	}

	if (area.has_value())
		tail->teamchain = area;

	return true;
}
#endif

void TargetTesla(entity &self, entity &tesla)
{
	// PMM - medic bails on healing things
	if (self.monsterinfo.aiflags & AI_MEDIC)
	{
#ifdef ROGUE_AI
		if (self.enemy.has_value())
			cleanupHealTarget(self.enemy);
#endif
		self.monsterinfo.aiflags &= ~AI_MEDIC;
	}

#ifdef ROGUE_AI
	// store the player enemy in case we lose track of him.
	if (self.enemy.has_value() && self.enemy->is_client())
		self.monsterinfo.last_player_enemy = self.enemy;
#endif

	if (self.enemy != tesla)
	{
		self.oldenemy = self.enemy;
		self.enemy = tesla;
		if (self.monsterinfo.attack)
		{
			if (self.health > 0)
				self.monsterinfo.attack(self);
		}
		else
			FoundTarget(self);
	}
}

// this returns a randomly selected coop player who is visible to self
// returns NULL if bad
entityref PickCoopTarget(entity &self)
{
	// if we're not in coop, this is a noop
	if (!coop)
		return null_entity;

	// no more than 4 players in coop, so..
	array<entityref, 4> targets;
	uint8_t num_targets = 0;

	for (entity &ent : entity_range(1, game.maxclients))
		if (ent.inuse && visible(self, ent))
			targets[num_targets++] = ent;

	if (!num_targets)
		return null_entity;

	return targets[Q_rand_uniform(num_targets)];
}

// only meant to be used in coop
uint8_t CountPlayers()
{
	// if we're not in coop, this is a noop
	if (!coop)
		return 1;

	uint8_t count = 0;

	for (entity &ent : entity_range(1, game.maxclients))
		if (ent.inuse)
			count++;

	return count;
}
#endif

#endif