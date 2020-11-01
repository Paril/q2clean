#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "../../lib/entity.h"
#include "../game.h"
#include "../util.h"
#include "ai.h"
#include "navigation.h"
#include "movement.h"
#include "aiweapons.h"
#include "aiitem.h"

//==========================================
// AI_Init
// Inits global parameters
//==========================================
void AI_Init()
{
	//Init developer mode
	bot_showpath = gi.cvar("bot_showpath", "0", CVAR_SERVERINFO);
	bot_showcombat = gi.cvar("bot_showcombat", "0", CVAR_SERVERINFO);
	bot_showsrgoal = gi.cvar("bot_showsrgoal", "0", CVAR_SERVERINFO);
	bot_showlrgoal = gi.cvar("bot_showlrgoal", "0", CVAR_SERVERINFO);
}

//==========================================
// AI_NewMap
// Inits Map local parameters
//==========================================
void AI_NewMap()
{
	//Load nodes
	AI_InitNavigationData();
	AI_InitAIWeapons ();
	AIDevel = {};
}


//==========================================
// AI_SetUpMoveWander
//==========================================
void AI_SetUpMoveWander(entity &ent)
{
	ent.g.ai.state = BOT_STATE_WANDER;
	ent.g.ai.wander_timeout_framenum = level.framenum + (gtime)(1.0 * BASE_FRAMERATE);
	ent.g.ai.nearest_node_tries = 0;
	
	ent.g.ai.next_move_framenum = level.framenum;
	ent.g.ai.bloqued_timeout_framenum = level.framenum + (gtime)(15.0 * BASE_FRAMERATE);
	
	ent.g.ai.goal_node = NODE_INVALID;
	ent.g.ai.current_node = NODE_INVALID;
	ent.g.ai.next_node = NODE_INVALID;
}

//==========================================
// AI_ResetWeights
// Init bot weights from bot-class weights.
//==========================================
void AI_ResetWeights(entity &ent)
{
	//restore defaults from bot persistant
	ent.g.ai.status.inventoryWeights = ent.g.ai.pers.inventoryWeights;
}


//==========================================
// AI_ResetNavigation
// Init bot navigation. Called at first spawn & each respawn
//==========================================
void AI_ResetNavigation(entity &ent)
{
	ent.g.enemy = 0;
	ent.g.movetarget = 0;
	ent.g.ai.state_combat_timeout_framenum = 0;

	ent.g.ai.state = BOT_STATE_WANDER;
	ent.g.ai.wander_timeout_framenum = level.framenum;
	ent.g.ai.nearest_node_tries = 0;

	ent.g.ai.next_move_framenum = level.framenum;
	ent.g.ai.bloqued_timeout_framenum = level.framenum + (gtime)(15.0 * BASE_FRAMERATE);

	ent.g.ai.goal_node = NODE_INVALID;
	ent.g.ai.current_node = NODE_INVALID;
	ent.g.ai.next_node = NODE_INVALID;
	
	ent.g.ai.move_vector = vec3_origin;

	//reset bot_roams timeouts
	ent.g.ai.status.broam_timeout_framenums.clear();
}

//==========================================
// AI_BotRoamForLRGoal
//
// Try assigning a bot roam node as LR Goal
//==========================================
bool AI_BotRoamForLRGoal(entity &self, node_id current_node)
{
	float	best_weight = 0.0;
	node_id	goal_node = NODE_INVALID;
	uint32_t best_broam = (uint32_t)-1;

	for (const auto &broam : nav.broams)
	{
		const uint32_t broam_index = &broam - nav.broams.data();

		if (self.g.ai.status.broam_timeout_framenums.contains(broam_index))
		{
			if (self.g.ai.status.broam_timeout_framenums[broam_index] > level.framenum)
				continue;

			self.g.ai.status.broam_timeout_framenums.erase(broam_index);
		}

		//limit cost finding by distance
		float dist = VectorDistance(self.s.origin, nav.nodes[broam.node].origin);

		// FIXME: 10000 is nearly max map size..
		if(dist > 10000)
			continue;

		//find cost
		float cost = AI_FindCost(current_node, broam.node, self.g.ai.pers.moveTypesMask);
		if (cost == NODE_INVALID || cost < 3) // ignore invalid and very short hops
			continue;

		cost *= random(); // Allow random variations for broams
		float weight = broam.weight / cost;	// Check against cost of getting there

		if (weight > best_weight)
		{
			best_weight = weight;
			goal_node = broam.node;
			best_broam = broam_index;
		}
	}

	if (best_broam == (uint32_t)-1 || !best_weight || !goal_node)
		return false;

	//set up the goal
	self.g.ai.state = BOT_STATE_MOVE;
	self.g.ai.tries = 0;	// Reset the count of how many times we tried this goal

	if (AIDevel.debugChased && bot_showlrgoal)
		gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected a bot roam of weight %f at node %d for LR goal.\n", self.client->g.pers.netname.ptr(), nav.broams[best_broam].weight, goal_node);

	AI_SetGoal(self, goal_node);
	return true;
}

//==========================================
// AI_PickLongRangeGoal
//
// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly. 
// Do not call it for every think cycle.
//
// jal: I don't think there is any problem by calling it,
// now that we have stored the costs at the nav.costs table (I don't do it anyway)
//==========================================
void AI_PickLongRangeGoal(entity &self)
{
	float	best_weight=0.0;
	node_id	goal_node = NODE_INVALID;
	entityref goal_ent;

	// look for a target
	node_id current_node = AI_FindClosestReachableNode(self.s.origin, self, (1 + self.g.ai.nearest_node_tries) * NODE_DENSITY, NODE_ALL);
	self.g.ai.current_node = current_node;

	if (current_node == NODE_INVALID)	//failed. Go wandering :(
	{
		gi.dprintf("%s: LRGOAL: Closest node not found. Tries:%i\n", self.client->g.pers.netname.ptr(), self.g.ai.nearest_node_tries);

		if (self.g.ai.state != BOT_STATE_WANDER)
			AI_SetUpMoveWander( self );

		self.g.ai.wander_timeout_framenum = level.framenum + (gtime)(1.0 * BASE_FRAMERATE);
		self.g.ai.nearest_node_tries++;	//extend search radius with each try
		return;
	}

	self.g.ai.nearest_node_tries = 0;

	// Items
	for (const auto &it : nav.items)
	{
		// Ignore items that are not there (solid)
		if (it.ent->solid == SOLID_NOT)
			continue;

		//ignore items wich can't be weighted (must have a valid item flag)
		if (!it.ent->g.item)
			continue;
		
		gitem_flags flags = it.ent->g.item->flags;
		
		if (!(flags & (IT_AMMO|IT_HEALTH|IT_ARMOR|IT_WEAPON|IT_POWERUP
#ifdef CTF
			|IT_FLAG|IT_TECH
#endif
			)))
			continue;

		float weight = AI_ItemWeight(self, it.ent);
		if (weight == 0.0f)	//ignore zero weighted items
			continue;

		//limit cost finding distance
		float dist = VectorDistance(self.s.origin, it.ent->s.origin);

		//different distance limits for different types
		if ((flags & (IT_AMMO
#ifdef CTF
			|IT_TECH
#endif
			)) && dist > 2000)
			continue;

		if ((flags & (IT_HEALTH|IT_ARMOR|IT_POWERUP)) && dist > 4000)
			continue;

		if ((flags & (IT_WEAPON
#ifdef CTF
			IT_FLAG
#endif
			)) && dist > 10000)
			continue;

		float cost = AI_FindCost(current_node, it.node, self.g.ai.pers.moveTypesMask);
		if (cost == NODE_INVALID || cost < 3) // ignore invalid and very short hops
			continue;

		//weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there

		if(weight > best_weight)
		{
			best_weight = weight;
			goal_node = it.node;
			goal_ent = it.ent;
		}
	}

	// Players: This should be its own function and is for now just finds a player to set as the goal.
	for (auto &player : entity_range(1, game.maxclients))
	{
		//ignore self & spectators
		if (!player.inuse || !player.client->g.pers.connected || player == self || (player.svflags & SVF_NOCLIENT))
			continue;

		//ignore zero weighted players
		if (!self.g.ai.status.playersWeights.contains(player.s.number))
			continue;

		node_id node = AI_FindClosestReachableNode(player.s.origin, player, NODE_DENSITY, NODE_ALL);
		float cost = AI_FindCost(current_node, node, self.g.ai.pers.moveTypesMask);

		if(cost == NODE_INVALID || cost < 4) // ignore invalid and very short hops
			continue;

		//precomputed player weights
		float weight = self.g.ai.status.playersWeights[player.s.number];

		//weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there

		if(weight > best_weight)
		{		
			best_weight = weight;
			goal_node = node;
			goal_ent = player;
		}
	}

	// If do not find a goal, go wandering....
	if(best_weight == 0.0 || goal_node == NODE_INVALID)
	{
		//BOT_ROAMS
		if (!AI_BotRoamForLRGoal(self, current_node))
		{
			self.g.ai.goal_node = NODE_INVALID;
			self.g.ai.state = BOT_STATE_WANDER;
			self.g.ai.wander_timeout_framenum = level.framenum + (gtime)(1.0 * BASE_FRAMERATE);
			gi.dprintf( "%s: did not find a LR goal, wandering.\n",self.client->g.pers.netname.ptr());
		}
		return; // no path?
	}

	// OK, everything valid, let's start moving to our goal.
	self.g.ai.state = BOT_STATE_MOVE;
	self.g.ai.tries = 0;	// Reset the count of how many times we tried this goal

	if (goal_ent.has_value())
		gi.dprintf( "%s: selected a %i at node %d for LR goal.\n",self.client->g.pers.netname.ptr(), goal_ent->g.type, goal_node);

	AI_SetGoal(self,goal_node);
}


//==========================================
// AI_PickShortRangeGoal
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
//==========================================
void AI_PickShortRangeGoal(entity &self)
{
	float		weight, best_weight=0.0;
	entityref	best = 0;

	// look for a target (should make more efficent later)
	entityref target = findradius(world, self.s.origin, AI_GOAL_SR_RADIUS);

	while (target.has_value())
	{
		if (!target->g.type)
			return;
		
		// Missile detection code
		if(target->g.type == ET_ROCKET || target->g.type == ET_GRENADE || target->g.type == ET_HANDGRENADE)
		{
			//if player who shoot is a potential enemy
			if (self.g.ai.status.playersWeights.contains(target->owner->s.number))
			{
				if(AIDevel.debugChased && bot_showcombat)
					gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: ROCKET ALERT!\n", self.client->g.pers.netname.ptr());
				
				self.g.enemy = target->owner;	// set who fired the rocket as enemy
				return;
			}
		}
		else if (target->g.type == ET_ITEM && AI_ItemIsReachable(self, target->s.origin))
		{
			if (infront(self, target))
			{
				weight = AI_ItemWeight(self, target);
				
				if(weight > best_weight)
				{
					best_weight = weight;
					best = target;
				}
			}
		}
		
		// next target
		target = findradius(target, self.s.origin, AI_GOAL_SR_RADIUS);	
	}
	
	//jalfixme (what's goalentity doing here?)
	if(best_weight)
	{
		self.g.movetarget = best;
		self.g.goalentity = best;
		if(AIDevel.debugChased && bot_showsrgoal && (self.g.goalentity != self.g.movetarget))
			gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected a %i for SR goal.\n",self.client->g.pers.netname.ptr(), self.g.movetarget->g.type);
	}
}

inline void AI_SetWaterLevel(entity &ent)
{
//
// get waterlevel
//
	vector point = ent.s.origin;
	point.z += ent.mins.z + 1;
	content_flags cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
	{
		ent.g.waterlevel = 0;
		ent.g.watertype = CONTENTS_NONE;
		return;
	}

	ent.g.watertype = cont;
	ent.g.waterlevel = 1;
	point.z += 26;
	cont = gi.pointcontents(point);
	if (!(cont & MASK_WATER))
		return;

	ent.g.waterlevel = 2;
	point.z += 22;
	cont = gi.pointcontents(point);
	if (cont & MASK_WATER)
		ent.g.waterlevel = 3;
};

//===================
//  AI_CategorizePosition
//  Categorize waterlevel and groundentity/stepping
//===================
inline void AI_CategorizePosition(entity &ent)
{
	bool stepping = AI_IsStep(ent);

	ent.g.ai.was_swim = ent.g.ai.is_swim;
	ent.g.ai.was_step = ent.g.ai.is_step;

	ent.g.ai.is_ladder = AI_IsLadder( ent.s.origin, ent.s.angles, ent.mins, ent.maxs, ent );

	AI_SetWaterLevel(ent);
	if (ent.g.waterlevel > 2 || (ent.g.waterlevel && !stepping))
	{
		ent.g.ai.is_swim = true;
		ent.g.ai.is_step = false;
		return;
	}

	ent.g.ai.is_swim = false;
	ent.g.ai.is_step = stepping;
}

//==========================================
// AI_Think
// think funtion for AIs
//==========================================
void AI_Think(entity &self)
{
	//AIDebug_SetChased(self);	//jal:debug shit
	AI_CategorizePosition(self);

	//freeze AI when dead
	if (self.g.deadflag)
	{
		self.g.ai.pers.deadFrame(self);
		return;
	}

	//if completely stuck somewhere
	if (VectorLength(self.g.velocity) > 37)
		self.g.ai.bloqued_timeout_framenum = level.framenum + (gtime)(10.0 * BASE_FRAMERATE);

	if (self.g.ai.bloqued_timeout_framenum < level.framenum)
	{
		self.g.ai.pers.bloquedTimeout(self);
		return;
	}

	//update status information to feed up ai
	self.g.ai.pers.UpdateStatus(self);

	//update position in path, set up move vector
	if (self.g.ai.state == BOT_STATE_MOVE)
	{
		if(!AI_FollowPath(self))
		{
			AI_SetUpMoveWander(self);
			self.g.ai.wander_timeout_framenum = level.framenum - (1 * BASE_FRAMERATE);	//do it now
		}
	}

	//pick a new long range goal
	if (self.g.ai.state == BOT_STATE_WANDER && self.g.ai.wander_timeout_framenum < level.framenum)
		AI_PickLongRangeGoal(self);

	//Find any short range goal
	AI_PickShortRangeGoal(self);

	//run class based states machine
	self.g.ai.pers.RunFrame(self);
}

#endif