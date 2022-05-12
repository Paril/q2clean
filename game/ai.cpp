#include "config.h"

#ifdef SINGLE_PLAYER
#include "entity.h"
#include "game.h"
#include "move.h"
#include "ai.h"
#include "game/weaponry.h"

#include "lib/gi.h"
#include "game/util.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "game/trail.h"
#include "game/rogue/ai.h"

#ifdef GROUND_ZERO
#include "game/rogue/ballistics/tesla.h"
#endif

//range
bool	enemy_vis;
float	enemy_range;
float	enemy_yaw;

//============================================================================

void AI_SetSightClient()
{
	uint32_t	start, check;

	if (!level.sight_client.has_value())
		start = 1;
	else
		start = level.sight_client->number;

	check = start;
	while (1)
	{
		check++;
		if (check > game.maxclients)
			check = 1;
		entity &ent = itoe(check);
		if (ent.inuse && ent.health > 0 && !(ent.flags & FL_VISIBLE_MASK))
		{
			level.sight_client = ent;
			return;     // got one
		}
		if (check == start)
		{
			level.sight_client = null_entity;
			return;     // nobody to see
		}
	}
}

//============================================================================

void ai_move(entity &self, float dist)
{
	M_walkmove(self, self.angles[YAW], dist);
}

// records sighting of an enemy
inline void ai_enemy_visible(entity &self)
{
	self.monsterinfo.aiflags &= ~AI_LOST_SIGHT;
	self.monsterinfo.last_sighting = self.enemy->origin;
	self.monsterinfo.trail_time = level.time;

#ifdef ROGUE_AI
	self.monsterinfo.blind_fire_target = self.enemy->origin;
	self.monsterinfo.blind_fire_framedelay = gtime::zero();
#endif
}

void ai_stand(entity &self, float dist)
{
	if (dist)
		M_walkmove(self, self.angles[YAW], dist);

	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
	{
		if (self.enemy.has_value())
		{
			self.ideal_yaw = vectoyaw(self.enemy->origin - self.origin);

			if (self.angles[YAW] != self.ideal_yaw && self.monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
			{
				self.monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
				self.monsterinfo.run(self);
			}
			
#if defined(ROGUE_AI) || defined(GROUND_ZERO)
			if (!(self.monsterinfo.aiflags & AI_MANUAL_STEERING))
#endif
				M_ChangeYaw (self);

			// find out if we're going to be shooting
			bool firing = ai_checkattack (self, 0);

			// record sightings of player
			if (self.enemy.has_value() && self.enemy->inuse && visible(self, self.enemy))
				ai_enemy_visible(self);
			else if (!firing)
			{
				FindTarget (self);
				return;
			}
		}
		else
			FindTarget(self);
		return;
	}

	if (FindTarget(self))
		return;

	if (level.time > self.monsterinfo.pause_time)
	{
		self.monsterinfo.walk(self);
		return;
	}

	if (!(self.spawnflags & 1) && self.monsterinfo.idle && (level.time > self.monsterinfo.idle_time))
	{
		if (self.monsterinfo.idle_time != gtime::zero())
		{
			self.monsterinfo.idle(self);
			self.monsterinfo.idle_time = level.time + random(15s, 30s);
		}
		else
			self.monsterinfo.idle_time = level.time + random(15s);
	}
}


void ai_walk(entity &self, float dist)
{
	M_MoveToGoal(self, dist);

	// check for noticing a player
	if (FindTarget(self))
		return;

	if (self.monsterinfo.search && (level.time > self.monsterinfo.idle_time))
	{
		if (self.monsterinfo.idle_time != gtime::zero())
		{
			self.monsterinfo.search(self);
			self.monsterinfo.idle_time = level.time + random(15s, 30s);
		}
		else
			self.monsterinfo.idle_time = level.time + random(15s);
	}
}


void ai_charge(entity &self, float dist)
{
	if (!self.enemy.has_value() || !self.enemy->inuse)
		return;
	
#ifdef ROGUE_AI
	// save blindfire target
	if (visible(self, self.enemy))
		self.monsterinfo.blind_fire_target = self.enemy->origin;
#endif

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	// PMM - made AI_MANUAL_STEERING affect things differently here .. they turn, but
	// don't set the ideal_yaw
	if (!(self.monsterinfo.aiflags & AI_MANUAL_STEERING))
#endif
		self.ideal_yaw = vectoyaw(self.enemy->origin - self.origin);

	M_ChangeYaw(self);

	if (dist)
	{
#ifdef ROGUE_AI
		if (self.monsterinfo.aiflags & AI_CHARGING)
		{
			M_MoveToGoal (self, dist);
			return;
		}
		// circle strafe support
		if (self.monsterinfo.attack_state == AS_SLIDING)
		{
			float ofs;

#ifdef GROUND_ZERO
			// if we're fighting a tesla, NEVER circle strafe
			if (self.enemy.has_value() && self.enemy->type == ET_TESLA)
				ofs = 0;
			else 
#endif
			if (self.monsterinfo.lefty)
				ofs = 90.f;
			else
				ofs = -90.f;
			
			if (M_walkmove (self, self.ideal_yaw + ofs, dist))
				return;
				
			self.monsterinfo.lefty = !self.monsterinfo.lefty;
			M_walkmove (self, self.ideal_yaw - ofs, dist);
		}
		else
#endif
			M_walkmove(self, self.angles[YAW], dist);
	}
}


void ai_turn(entity &self, float dist)
{
	if (dist)
		M_walkmove(self, self.angles[YAW], dist);

	if (FindTarget(self))
		return;

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	if (!(self.monsterinfo.aiflags & AI_MANUAL_STEERING))
#endif
		M_ChangeYaw(self);
}

//============================================================================
void AttackFinished(entity &self, gtimef time)
{
	self.monsterinfo.attack_finished = duration_cast<gtime>(level.time + time);
}

void HuntTarget(entity &self)
{	
	self.goalentity = self.enemy;

	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		self.monsterinfo.stand(self);
	else
		self.monsterinfo.run(self);

	self.ideal_yaw = vectoyaw(self.enemy->origin - self.origin);

	// wait a while before first attack
	if (!(self.monsterinfo.aiflags & AI_STAND_GROUND))
		AttackFinished(self, 1s);
}

void FoundTarget(entity &self)
{
	// let other monsters see this monster for a while
	if (self.enemy->is_client)
	{
#ifdef GROUND_ZERO
		self.enemy->flags &= ~FL_DISGUISED;
#endif
		
		level.sight_entity = self;
		level.sight_entity_time = level.time;
	}

	self.show_hostile = level.time + 1s;   // wake up other monsters

	self.monsterinfo.last_sighting = self.enemy->origin;
	self.monsterinfo.trail_time = level.time;
	
#ifdef ROGUE_AI
	self.monsterinfo.blind_fire_target = self.enemy->origin;
	self.monsterinfo.blind_fire_framedelay = gtime::zero();
#endif

	if (!self.combattarget)
	{
		HuntTarget(self);
		return;
	}

	self.goalentity = self.movetarget = G_PickTarget(self.combattarget);
	if (!self.movetarget.has_value())
	{
		self.goalentity = self.movetarget = self.enemy;
		HuntTarget(self);
		gi.dprintfmt("{}: combattarget {} not found\n", self, self.combattarget);
		return;
	}

	// clear out our combattarget, these are a one shot deal
	self.combattarget = nullptr;
	self.monsterinfo.aiflags |= AI_COMBAT_POINT;

	// clear the targetname, that point is ours!
	self.movetarget->targetname = nullptr;
	self.monsterinfo.pause_time = gtime::zero();

	// run for it
	self.monsterinfo.run(self);
}

// temp
static entity_type ET_TARGET_ACTOR;

bool FindTarget(entity &self)
{
	if (self.monsterinfo.aiflags & AI_GOOD_GUY) {
		if (self.goalentity.has_value() && self.goalentity->inuse && self.goalentity->type) {
			if (self.goalentity->type == ET_TARGET_ACTOR)
				return false;
		}

		//FIXME look for monsters?
		return false;
	}

	// if we're going to a combat point, just proceed
	if (self.monsterinfo.aiflags & AI_COMBAT_POINT)
		return false;

// if the first spawnflag bit is set, the monster will only wake up on
// really seeing the player, not another monster getting angry or hearing
// something

// revised behavior so they will wake up if they "see" a player make a noise
// but not weapon impact/explosion noises

	bool heardit = false;
	entityref	cl;

	if ((level.sight_entity_time >= (level.time - 1_hz)) && !(self.spawnflags & 1))
	{
		cl = level.sight_entity;
		if (cl->enemy == self.enemy)
			return false;
	}
#ifdef GROUND_ZERO
	else if (level.disguise_violation_time > level.time)
		cl = level.disguise_violator;
#endif
	else if (level.sound_entity_time >= (level.time - 1_hz)) {
		cl = level.sound_entity;
		heardit = true;
	} else if (!(self.enemy.has_value()) && (level.sound2_entity_time >= (level.time - 1_hz)) && !(self.spawnflags & 1)) {
		cl = level.sound2_entity;
		heardit = true;
	} else {
		cl = level.sight_client;
		if (!cl.has_value())
			return false;   // no clients to get mad at
	}

	// if the entity went away, forget it
	if (!cl->inuse)
		return false;

	if (cl == self.enemy)
		return true;    // JDC false;

#ifdef ROGUE_AI
	if ((self.monsterinfo.aiflags & AI_HINT_PATH) && coop)
		heardit = false;
#endif

	if (cl->is_client) {
		if (cl->flags & FL_NOTARGET)
			return false;
	} else if (cl->svflags & SVF_MONSTER) {
		if (!cl->enemy.has_value())
			return false;
		if (cl->enemy->flags & FL_NOTARGET)
			return false;
	} else if (heardit) {
		if (cl->owner.has_value() && cl->owner->flags & FL_NOTARGET)
			return false;
	} else
		return false;

	if (!heardit) {
		float r = VectorDistance(self.origin, cl->origin);

		if (r >= RANGE_MID)
			return false;

// this is where we would check invisibility

		if (!visible(self, cl)) {
			return false;
		}

		if (r < RANGE_NEAR) {
			if (cl->show_hostile < level.time && !infront(self, cl)) {
				return false;
			}
		} else if (r < RANGE_MID) {
			if (!infront(self, cl)) {
				return false;
			}
		}

		self.enemy = cl;

		if (self.enemy->type != ET_PLAYER_NOISE) {
			self.monsterinfo.aiflags &= ~AI_SOUND_TARGET;

			if (!self.enemy->is_client) {
				self.enemy = self.enemy->enemy;
				if (!self.enemy->is_client) {
					self.enemy = null_entity;
					return false;
				}
			}
		}
	} else { // heardit
		if (self.spawnflags & 1) {
			if (!visible(self, cl))
				return false;
		} else {
			if (!gi.inPHS(self.origin, cl->origin))
				return false;
		}

		vector temp = cl->origin - self.origin;

		if (VectorLength(temp) > 1000) { // too far to hear
			return false;
		}

		// check area portals - if they are different and not connected then we can't hear it
		if (cl->areas[0] != self.areas[0] && !gi.AreasConnected(self.areas[0], cl->areas[0]))
			return false;

		self.ideal_yaw = vectoyaw(temp);
		
#if defined(ROGUE_AI) || defined(GROUND_ZERO)
		if (!(self.monsterinfo.aiflags & AI_MANUAL_STEERING))
#endif
			M_ChangeYaw(self);

		// hunt the sound for a bit; hopefully find the real player
		self.monsterinfo.aiflags |= AI_SOUND_TARGET;
		self.enemy = cl;
	}

//
// got one
//
#ifdef ROGUE_AI
	// PMM - if we got an enemy, we need to bail out of hint paths, so take over here
	if (self.monsterinfo.aiflags & AI_HINT_PATH)
		// this calls foundtarget for us
		hintpath_stop (self);
	else
#endif
		FoundTarget(self);

	if (!(self.monsterinfo.aiflags & AI_SOUND_TARGET) && (self.monsterinfo.sight))
		self.monsterinfo.sight(self, self.enemy);

	return true;
}


//=============================================================================

/*
============
FacingIdeal

============
*/
static bool FacingIdeal(entity &self)
{
	float   delta;

	delta = anglemod(self.angles[YAW] - self.ideal_yaw);
	if (delta > 45 && delta < 315)
		return false;
	return true;
}


//=============================================================================

#ifdef GROUND_ZERO
static entity_type ET_MONSTER_DAEDALUS;
#endif

bool M_CheckAttack(entity &self)
{
	vector	spot1, spot2;
	float	chance;
	trace	tr;

	if (self.enemy->health > 0) {
		// see if any entities are in the way of the shot
		spot1 = self.origin;
		spot1.z += self.viewheight;
		spot2 = self.enemy->origin;
		spot2.z += self.enemy->viewheight;

		tr = gi.traceline(spot1, spot2, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WINDOW);

		// do we have a clear shot?
		if (tr.ent != self.enemy)
		{
#ifdef ROGUE_AI
			// PGM - we want them to go ahead and shoot at info_notnulls if they can.
			if(self.enemy->solid != SOLID_NOT || tr.fraction < 1.0)
			{
				// PMM - if we can't see our target, and we're not blocked by a monster, go into blind fire if available
				if (!(tr.ent.svflags & SVF_MONSTER) && !visible(self, self.enemy))
				{
					if (self.monsterinfo.blindfire && (self.monsterinfo.blind_fire_framedelay <= 20s))
					{
						if (level.time < self.monsterinfo.attack_finished)
							return false;

						if (level.time < (self.monsterinfo.trail_time + self.monsterinfo.blind_fire_framedelay))
							// wait for our time
							return false;

						// make sure we're not going to shoot a monster
						tr = gi.traceline(spot1, self.monsterinfo.blind_fire_target, self, CONTENTS_MONSTER);
						if (tr.allsolid || tr.startsolid || ((tr.fraction < 1.0) && (tr.ent != self.enemy)))
							return false;

						self.monsterinfo.attack_state = AS_BLIND;
						return true;
					}
				}
				// pmm
#endif
				return false;
#ifdef ROGUE_AI
			}
#endif
		}
	}

	// melee attack
	if (enemy_range <= RANGE_MELEE)
	{
		self.monsterinfo.attack_state = AS_STRAIGHT;

		// don't always melee in easy mode
		if (!skill && random() > 0.25f)
			return false;

		if (self.monsterinfo.melee && (!self.monsterinfo.attack || !(Q_rand_uniform(4) % (skill + 1))))
			self.monsterinfo.attack_state = AS_MELEE;
		else
			self.monsterinfo.attack_state = AS_MISSILE;

		return true;
	}

// missile attack
	if (!self.monsterinfo.attack)
	{
		self.monsterinfo.attack_state = AS_STRAIGHT;
		return false;
	}

	if (level.time < self.monsterinfo.attack_finished)
		return false;

	if (enemy_range > RANGE_MID)
		return false;

	if (self.monsterinfo.aiflags & AI_STAND_GROUND) {
		chance = 0.4f;
	} else if (enemy_range < RANGE_MELEE) {
		chance = 0.2f;
	} else if (enemy_range < RANGE_NEAR) {
		chance = 0.1f;
	} else if (enemy_range < RANGE_MID) {
		chance = 0.02f;
	} else {
		return false;
	}

	if (!skill)
		chance *= 0.5f;
	else if (skill >= 2)
		chance *= 2;

	if (random() < chance || self.enemy->solid == SOLID_NOT)
	{
		self.monsterinfo.attack_state = AS_MISSILE;
		self.monsterinfo.attack_finished = level.time + random(2s);
		return true;
	}

	if (self.flags & FL_FLY)
	{
#ifdef ROGUE_AI
		// originally, just 0.3
		float strafe_chance;
		
		// if enemy is tesla, never strafe
#ifdef GROUND_ZERO
		if (self.enemy.has_value() && self.enemy->type == ET_TESLA)
			strafe_chance = 0;
		else if (self.type == ET_MONSTER_DAEDALUS)
			strafe_chance = 0.8f;
		else
#endif
			strafe_chance = 0.6f;


		if (random() < strafe_chance)
#else
		if (random() < 0.3f)
#endif
			self.monsterinfo.attack_state = AS_SLIDING;
		else
			self.monsterinfo.attack_state = AS_STRAIGHT;
	}

#ifdef ROGUE_AI
	else
	{
		if (random() < 0.4)
			self.monsterinfo.attack_state = AS_SLIDING;
		else
			self.monsterinfo.attack_state = AS_STRAIGHT;
	}
#endif

	return false;
}

REGISTER_SAVABLE(M_CheckAttack);

/*
=============
ai_run_melee

Turn and close until within an angle to launch a melee attack
=============
*/
static void ai_run_melee(entity &self)
{
	self.ideal_yaw = enemy_yaw;

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	if (!(self.monsterinfo.aiflags & AI_MANUAL_STEERING))
#endif
		M_ChangeYaw(self);

	if (FacingIdeal(self))
	{
		self.monsterinfo.melee(self);
		self.monsterinfo.attack_state = AS_STRAIGHT;
	}
}


/*
=============
ai_run_missile

Turn in place until within an angle to launch a missile attack
=============
*/
static void ai_run_missile(entity &self)
{
	self.ideal_yaw = enemy_yaw;

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	if (!(self.monsterinfo.aiflags & AI_MANUAL_STEERING))
#endif
		M_ChangeYaw(self);

	if (FacingIdeal(self))
	{
		self.monsterinfo.attack(self);

#if defined(ROGUE_AI) || defined(GROUND_ZERO)	
		if (self.monsterinfo.attack_state == AS_MISSILE || self.monsterinfo.attack_state == AS_BLIND)
#endif
			self.monsterinfo.attack_state = AS_STRAIGHT;
	}
}

#ifdef ROGUE_AI
static constexpr float MAX_SIDESTEP	= 8.f;
#endif

/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
static void ai_run_slide(entity &self, float distance)
{
	float   ofs;

	self.ideal_yaw = enemy_yaw;

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	if (!(self.monsterinfo.aiflags & AI_MANUAL_STEERING))
		M_ChangeYaw(self);

#endif
#ifdef ROGUE_AI
	if (!(self.flags & FL_FLY))
		distance = min(distance, MAX_SIDESTEP);

#endif
	if (self.monsterinfo.lefty)
		ofs = 90.f;
	else
		ofs = -90.f;

	if (M_walkmove(self, self.ideal_yaw + ofs, distance))
		return;

#ifdef ROGUE_AI
	// PMM - if we're dodging, give up on it and go straight
	if (self.monsterinfo.aiflags & AI_DODGING)
	{
		monster_done_dodge (self);
		// by setting as_straight, caller will know to try straight move
		self.monsterinfo.attack_state = AS_STRAIGHT;
		return;
	}
#endif

	self.monsterinfo.lefty = !self.monsterinfo.lefty;

#ifdef ROGUE_AI
	if (M_walkmove (self, self.ideal_yaw - ofs, distance))
		return;

	// if we're dodging, give up on it and go straight
	if (self.monsterinfo.aiflags & AI_DODGING)
		monster_done_dodge (self);

	// the move failed, so signal the caller (ai_run) to try going straight
	self.monsterinfo.attack_state = AS_STRAIGHT;
#else
	M_walkmove(self, self.ideal_yaw - ofs, distance);
#endif
}


bool ai_checkattack(entity &self, float)
{
// this causes monsters to run blindly to the combat point w/o firing
	if (self.goalentity.has_value())
	{
		if (self.monsterinfo.aiflags & AI_COMBAT_POINT)
			return false;

		if (self.monsterinfo.aiflags & AI_SOUND_TARGET)
		{
			if ((level.time - self.enemy->last_sound_time) > 5s)
			{
				if (self.goalentity == self.enemy)
				{
					if (self.movetarget.has_value())
						self.goalentity = self.movetarget;
					else
						self.goalentity = null_entity;
				}
				self.monsterinfo.aiflags &= ~AI_SOUND_TARGET;
				if (self.monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
					self.monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			}
			else
			{
				self.show_hostile = level.time + 1s;
				return false;
			}
		}
	}

	enemy_vis = false;

// see if the enemy is dead
	bool hesDeadJim = false;

	if ((!self.enemy.has_value()) || (!self.enemy->inuse))
		hesDeadJim = true;
	else if (self.monsterinfo.aiflags & AI_MEDIC)
	{
		if (!(self.enemy->inuse) || self.enemy->health > 0)
			hesDeadJim = true;
	}
	else
	{
		if (self.monsterinfo.aiflags & AI_BRUTAL)
		{
			if (self.enemy->health <= -80)
				hesDeadJim = true;
		} 
		else
		{
			if (self.enemy->health <= 0)
				hesDeadJim = true;
		}
	}

	if (hesDeadJim)
	{
		self.monsterinfo.aiflags &= ~AI_MEDIC;
		self.enemy = 0;
		// FIXME: look all around for other targets
		if (self.oldenemy.has_value() && self.oldenemy->health > 0)
		{
			self.enemy = self.oldenemy;
			self.oldenemy = 0;
			HuntTarget(self);
		}
#ifdef ROGUE_AI
		else if (self.monsterinfo.last_player_enemy.has_value() && self.monsterinfo.last_player_enemy->health > 0)
		{
			self.enemy = self.monsterinfo.last_player_enemy;
			self.oldenemy = 0;
			self.monsterinfo.last_player_enemy = 0;
			HuntTarget (self);
		}
#endif
		else
		{
			if (self.movetarget.has_value()) {
				self.goalentity = self.movetarget;
				self.monsterinfo.walk(self);
			} else {
				// we need the pausetime otherwise the stand code
				// will just revert to walking with no target and
				// the monsters will wonder around aimlessly trying
				// to hunt the world entity
				self.monsterinfo.pause_time = gtime::max();
				self.monsterinfo.stand(self);
			}
			return true;
		}
	}

	self.show_hostile = level.time + 1s;   // wake up other monsters

// check knowledge of enemy
	enemy_vis = visible(self, self.enemy);
	if (enemy_vis)
	{
		self.monsterinfo.search_time = level.time + 5s;
		ai_enemy_visible(self);
	}

	vector temp = self.enemy->origin - self.origin;
	enemy_range = VectorLength(temp);
	enemy_yaw = vectoyaw(temp);
	
#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	// PMM -- reordered so the monster specific checkattack is called before the run_missle/melee/checkvis
	// stuff .. this allows for, among other things, circle strafing and attacking while in ai_run
	bool retval = self.monsterinfo.checkattack (self);

	if (!retval)
		return false;
#endif

	// if enemy is not currently visible, we will never attack
	if (!enemy_vis)
		return false;

	if (self.monsterinfo.attack_state == AS_MISSILE
#if defined(ROGUE_AI) || defined(GROUND_ZERO)
		|| self.monsterinfo.attack_state == AS_BLIND
#endif
		)
	{
		ai_run_missile(self);
		return true;
	}
	else if (self.monsterinfo.attack_state == AS_MELEE)
	{
		ai_run_melee(self);
		return true;
	}

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	return true;
#else
	return self.monsterinfo.checkattack(self);
#endif
}

void ai_run(entity &self, float dist)
{
	bool	isNew;
	float	d1, d2;
	trace	tr;
	vector	v_forward, v_right;
	float	left, center, right;
	vector	left_target, right_target;
	bool	alreadyMoved = false;

	// if we're going to a combat point, just proceed
	if (self.monsterinfo.aiflags & AI_COMBAT_POINT)
	{
		M_MoveToGoal(self, dist);
		return;
	}

#ifdef ROGUE_AI
	bool		gotcha = false;
	entityref	realEnemy;

	if (self.monsterinfo.aiflags & AI_DUCKED)
		self.monsterinfo.aiflags &= ~AI_DUCKED;
	if (self.bounds.maxs[2] != self.monsterinfo.base_height)
		monster_duck_up (self);

	// if we're currently looking for a hint path
	if (self.monsterinfo.aiflags & AI_HINT_PATH)
	{
		M_MoveToGoal (self, dist);

		if (!self.inuse)
			return;

		// first off, make sure we're looking for the player, not a noise he made
		if (self.enemy.has_value())
		{
			if (self.enemy->inuse)
			{
				if (self.enemy->type != ET_PLAYER_NOISE)
					realEnemy = self.enemy;
				else if (self.enemy->owner.has_value())
					realEnemy = self.enemy->owner;
				else // uh oh, can't figure out enemy, bail
				{
					self.enemy = 0;
					hintpath_stop (self);
					return;
				}
			}
			else
			{
				self.enemy = 0;
				hintpath_stop (self);
				return;
			}
		}
		else
		{
			hintpath_stop (self);
			return;
		}

		if (coop)
		{
			// if we're in coop, check my real enemy first .. if I SEE him, set gotcha to true
			if (self.enemy.has_value() && visible(self, realEnemy))
				gotcha = true;
			else // otherwise, let FindTarget bump us out of hint paths, if appropriate
				FindTarget(self);
		}
		else if (self.enemy.has_value() && visible(self, realEnemy))
			gotcha = true;
		
		// if we see the player, disconnect from hintpaths and start looking normally.
		if (gotcha)
			hintpath_stop (self);

		return;
	}
#endif

	if (self.monsterinfo.aiflags & AI_SOUND_TARGET)
	{
		if (!self.enemy.has_value() || VectorLength(self.origin - self.enemy->origin) < 64)
		{
			self.monsterinfo.aiflags |= (AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			self.monsterinfo.stand(self);
			return;
		}

		M_MoveToGoal(self, dist);

		alreadyMoved = true;

		if (!self.inuse)
			return;

		if (!FindTarget(self))
			return;
	}

#ifdef ROGUE_AI
	bool retval = ai_checkattack (self, dist);

	// don't strafe if we can't see our enemy
	if (!enemy_vis && self.monsterinfo.attack_state == AS_SLIDING)
		self.monsterinfo.attack_state = AS_STRAIGHT;
	// unless we're dodging (dodging out of view looks smart)
	if (self.monsterinfo.aiflags & AI_DODGING)
		self.monsterinfo.attack_state = AS_SLIDING;
#else
	if (ai_checkattack(self, dist))
		return;
#endif

	if (self.monsterinfo.attack_state == AS_SLIDING)
	{
		// protect against double moves
		if (!alreadyMoved)
			ai_run_slide(self, dist);

#ifdef ROGUE_AI
		// we're using attack_state as the return value out of ai_run_slide to indicate whether or not the
		// move succeeded.  If the move succeeded, and we're still sliding, we're done in here (since we've
		// had our chance to shoot in ai_checkattack, and have moved).
		// if the move failed, our state is as_straight, and it will be taken care of below
		if (!retval && self.monsterinfo.attack_state == AS_SLIDING)
#endif
			return;
	}
#ifdef ROGUE_AI
	else if (self.monsterinfo.aiflags & AI_CHARGING)
	{
		self.ideal_yaw = enemy_yaw;
		if (!(self.monsterinfo.aiflags & AI_MANUAL_STEERING))
			M_ChangeYaw (self);
	}

	if (retval)
	{
		// PMM - is this useful? Monsters attacking usually call the ai_charge routine..
		// the only monster this affects should be the soldier
		if (dist != 0 && !alreadyMoved && self.monsterinfo.attack_state == AS_STRAIGHT && !(self.monsterinfo.aiflags & AI_STAND_GROUND))
			M_MoveToGoal (self, dist);

		if (self.enemy.has_value() && self.enemy->inuse && enemy_vis)
			ai_enemy_visible(self);

		return;
	}
#endif

	if (self.enemy.has_value() && self.enemy->inuse && enemy_vis)
	{
		if (!alreadyMoved)
			M_MoveToGoal(self, dist);

		if (!self.inuse)
			return;

		ai_enemy_visible(self);
		return;
	}

#ifdef ROGUE_AI
	// if we've been looking (unsuccessfully) for the player for 5 seconds
	if ((self.monsterinfo.trail_time + 5s) <= level.time)
	{
		// and we haven't checked for valid hint paths in the last 10 seconds
		if ((self.monsterinfo.last_hint_time + 10s) <= level.time)
		{
			// check for hint_paths.
			self.monsterinfo.last_hint_time = level.time;

			if (monsterlost_checkhint(self))
				return;
		}
	}
#endif

	// coop will change to another enemy if visible
	if (coop)
	{
		// FIXME: insane guys get mad with this, which causes crashes!
		if (FindTarget(self))
			return;
	}

	if (self.monsterinfo.search_time != gtime::zero() && (level.time > (self.monsterinfo.search_time + 20s)))
	{
		if (!alreadyMoved)
			M_MoveToGoal(self, dist);
		self.monsterinfo.search_time = gtime::zero();
		return;
	}

	entityref save = self.goalentity;
	entityref marker;
	entity &tempgoal = G_Spawn();
	self.goalentity = tempgoal;

	isNew = false;

	if (!(self.monsterinfo.aiflags & AI_LOST_SIGHT)) {
		// just lost sight of the player, decide where to go first
		self.monsterinfo.aiflags |= (AI_LOST_SIGHT | AI_PURSUIT_LAST_SEEN);
		self.monsterinfo.aiflags &= ~(AI_PURSUE_NEXT | AI_PURSUE_TEMP);
		isNew = true;
	}

	if (self.monsterinfo.aiflags & AI_PURSUE_NEXT) {
		self.monsterinfo.aiflags &= ~AI_PURSUE_NEXT;

		// give ourself more time since we got this far
		self.monsterinfo.search_time = level.time + 5s;

		if (self.monsterinfo.aiflags & AI_PURSUE_TEMP) {
			self.monsterinfo.aiflags &= ~AI_PURSUE_TEMP;
			marker = 0;
			self.monsterinfo.last_sighting = self.monsterinfo.saved_goal;
			isNew = true;
		} else if (self.monsterinfo.aiflags & AI_PURSUIT_LAST_SEEN) {
			self.monsterinfo.aiflags &= ~AI_PURSUIT_LAST_SEEN;
			marker = PlayerTrail_PickFirst(self);
		} else {
			marker = PlayerTrail_PickNext(self);
		}

		if (marker.has_value()) {
			self.monsterinfo.last_sighting = marker->origin;
			self.monsterinfo.trail_time = marker->timestamp;
			self.angles[YAW] = self.ideal_yaw = marker->angles[YAW];

			isNew = true;
		}
	}

	vector v = self.origin - self.monsterinfo.last_sighting;
	d1 = VectorLength(v);
	if (d1 <= dist) {
		self.monsterinfo.aiflags |= AI_PURSUE_NEXT;
		dist = d1;
	}

	self.goalentity->origin = self.monsterinfo.last_sighting;

	if (isNew) {
		tr = gi.trace(self.origin, self.bounds, self.monsterinfo.last_sighting, self, MASK_PLAYERSOLID);
		if (tr.fraction < 1) {
			v = self.goalentity->origin - self.origin;
			d1 = VectorLength(v);
			center = tr.fraction;
			d2 = d1 * ((center + 1) / 2);
			self.angles[YAW] = self.ideal_yaw = vectoyaw(v);
			AngleVectors(self.angles, &v_forward, &v_right, nullptr);

			v = { d2, -16, 0 };
			left_target = G_ProjectSource(self.origin, v, v_forward, v_right);
			tr = gi.trace(self.origin, self.bounds, left_target, self, MASK_PLAYERSOLID);
			left = tr.fraction;

			v = { d2, 16, 0 };
			right_target = G_ProjectSource(self.origin, v, v_forward, v_right);
			tr = gi.trace(self.origin, self.bounds, right_target, self, MASK_PLAYERSOLID);
			right = tr.fraction;

			center = (d1 * center) / d2;
			if (left >= center && left > right)
			{
				if (left < 1)
				{
					v = { d2 * left * 0.5f, -16, 0 };
					left_target = G_ProjectSource(self.origin, v, v_forward, v_right);
				}
				self.monsterinfo.saved_goal = self.monsterinfo.last_sighting;
				self.monsterinfo.aiflags |= AI_PURSUE_TEMP;
				self.goalentity->origin = left_target;
				self.monsterinfo.last_sighting = left_target;
				v = self.goalentity->origin - self.origin;
				self.angles[YAW] = self.ideal_yaw = vectoyaw(v);
			}
			else if (right >= center && right > left)
			{
				if (right < 1)
				{
					v = { d2 * right * 0.5f, 16, 0 };
					right_target = G_ProjectSource(self.origin, v, v_forward, v_right);
				}
				self.monsterinfo.saved_goal = self.monsterinfo.last_sighting;
				self.monsterinfo.aiflags |= AI_PURSUE_TEMP;
				self.goalentity->origin = right_target;
				self.monsterinfo.last_sighting = right_target;
				v = self.goalentity->origin - self.origin;
				self.angles[YAW] = self.ideal_yaw = vectoyaw(v);
			}
		}
	}

	M_MoveToGoal(self, dist);

	G_FreeEdict(tempgoal);

	// it's possible to be freed from touching a trigger
	if (!self.inuse)
		return;

	if (self.inuse)
		self.goalentity = save;
}

#endif