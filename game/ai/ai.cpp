#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "../../lib/entity.h"
#include "../player.h"
#include "../game.h"
#include "../pweapon.h"
#include "ai.h"
#include "links.h"
#include "movement.h"
#include "navigation.h"
#include "aiweapons.h"
#include "aiitem.h"

cvarref bot_showpath;
cvarref bot_showcombat;
cvarref bot_showsrgoal;
cvarref bot_showlrgoal;

nav_data nav;

ai_devel AIDevel;

//==========================================
// BOT_DMclass_Move
// DMClass is generic bot class
//==========================================
static void BOT_DMclass_Move(entity &self, usercmd &ucmd)
{
	ai_link_type	current_link_type = LINK_NONE;

	node_flags next_node_flags = nav.nodes[self.g.ai.next_node].flags;

	if( AI_PlinkExists( self.g.ai.current_node, self.g.ai.next_node ))
		current_link_type = AI_PlinkMoveType( self.g.ai.current_node, self.g.ai.next_node );

	// Platforms
	if( current_link_type == LINK_PLATFORM )
	{
		// Move to the center
		self.g.ai.move_vector[2] = 0; // kill z movement
		if(VectorLength(self.g.ai.move_vector) > 10)
			ucmd.forwardmove = 200; // walk to center

		AI_ChangeAngle(self);

		return; // No move, riding elevator
	}
	else if( next_node_flags & NODEFLAGS_PLATFORM )
	{
		// is lift down?
		for (const auto &ent : nav.ents)
			if(ent.node == self.g.ai.next_node )
				//if not reachable, wait for it (only height matters)
				if( (ent.ent->s.origin[2] + ent.ent->maxs[2])
					> (self.s.origin[2] + self.mins[2] + AI_JUMPABLE_HEIGHT) )
					return; //wait for elevator
	}

	// Ladder movement
	if( self.g.ai.is_ladder )
	{
		ucmd.forwardmove = 70;
		ucmd.upmove = 200;
		ucmd.sidemove = 0;
		return;
	}

	// Falling off ledge
	if(!self.g.groundentity.has_value() && !self.g.ai.is_step && !self.g.ai.is_swim )
	{
		AI_ChangeAngle(self);
		if (current_link_type == LINK_JUMPPAD )
			ucmd.forwardmove = 100;
		else if( current_link_type == LINK_JUMP )
		{
			self.g.velocity[0] = self.g.ai.move_vector[0] * 280;
			self.g.velocity[1] = self.g.ai.move_vector[1] * 280;
		}
		else
		{
			self.g.velocity[0] = self.g.ai.move_vector[0] * 160;
			self.g.velocity[1] = self.g.ai.move_vector[1] * 160;
		}
		return;
	}

	// jumping over (keep fall before this)
	if( current_link_type == LINK_JUMP && !self.g.groundentity.has_value()) 
	{
		vector  v1, v2;
		//check floor in front, if there's none... Jump!
		v1 = self.s.origin;
		v2 = self.g.ai.move_vector;
		VectorNormalize( v2 );
		v1 += 12 * v2;
		v1[2] += self.mins[2];
		trace tr = gi.trace(v1, { -2, -2, -AI_JUMPABLE_HEIGHT }, { 2, 2, 0 }, v1, self, MASK_AISOLID );
		if( !tr.startsolid && tr.fraction == 1.0 )
		{
			//jump!
			ucmd.forwardmove = 400;
			//prevent double jumping on crates
			v1 = self.s.origin;
			v1[2] += self.mins[2];
			tr = gi.trace(v1, { -12, -12, -8 }, { 12, 12, 0 }, v1, self, MASK_AISOLID );
			if( tr.startsolid )
				ucmd.upmove = 400;
			return;
		}
	}

	// Move To Short Range goal (not following paths)
	// plats, grapple, etc have higher priority than SR Goals, cause the bot will 
	// drop from them and have to repeat the process from the beginning
	if (AI_MoveToGoalEntity(self,ucmd))
		return;

	// swimming
	if(self.g.ai.is_swim)
	{
		// We need to be pointed up/down
		AI_ChangeAngle(self);

		if (!(gi.pointcontents(nav.nodes[self.g.ai.next_node].origin) & MASK_WATER)) // Exit water
			ucmd.upmove = 400;
		
		ucmd.forwardmove = 300;
		return;
	}

	// Check to see if stuck, and if so try to free us
 	if(VectorLength(self.g.velocity) < 37)
	{
		// Keep a random factor just in case....
		if(random() > 0.1 && AI_SpecialMove(self, ucmd)) //jumps, crouches, turns...
			return;

		self.s.angles[YAW] += random(-90.f, 90.f);

		AI_ChangeAngle(self);

		ucmd.forwardmove = 400;

		return;
	}

	AI_ChangeAngle(self);

	// Otherwise move as fast as we can... 
	ucmd.forwardmove = 400;
}

//==========================================
// BOT_DMclass_Wander
// Wandering code (based on old ACE movement code) 
//==========================================
static void BOT_DMclass_Wander(entity &self, usercmd &ucmd)
{
	// Do not move
	if(self.g.ai.next_move_framenum > level.framenum)
		return;

	if (self.g.deadflag)
		return;

	// Special check for elevators, stand still until the ride comes to a complete stop.
	if(self.g.groundentity.has_value() && self.g.groundentity->g.type == ET_FUNC_PLAT)
	{
		if (self.g.groundentity->g.moveinfo.state == STATE_UP ||
		   self.g.groundentity->g.moveinfo.state == STATE_DOWN)
		{
			self.g.velocity[0] = 0;
			self.g.velocity[1] = 0;
			self.g.velocity[2] = 0;
			self.g.ai.next_move_framenum = level.framenum + (gtime)(0.5 * BASE_FRAMERATE);
			return;
		}
	}

	// Move To Goal (Short Range Goal, not following paths)
	if (AI_MoveToGoalEntity(self,ucmd))
		return;
	
	// Swimming?
	vector temp = self.s.origin;
	temp[2]+=24;

	if( gi.pointcontents (temp) & MASK_WATER)
	{
		// If drowning and no node, move up
		if (self.client->g.next_drown_framenum > 0 )	//jalfixme: client references must pass into botStatus
		{
			ucmd.upmove = 100;
			self.s.angles[PITCH] = -45;
		}
		else
			ucmd.upmove = 15;

		ucmd.forwardmove = 300;
	}

	// Lava?
	temp[2]-=48;

	if( gi.pointcontents(temp) & (CONTENTS_LAVA|CONTENTS_SLIME) )
	{
		self.s.angles[YAW] += random(-180.f, 180.f);
		ucmd.forwardmove = 400;
		if(self.g.groundentity.has_value())
			ucmd.upmove = 400;
		else
			ucmd.upmove = 0;
		return;
	}

	// Check for special movement
 	if(VectorLength(self.g.velocity) < 37)
	{
		if(random() > 0.1 && AI_SpecialMove(self,ucmd))	//jumps, crouches, turns...
			return;

		self.s.angles[YAW] += random(-90.f, 90.f);
 
		if (!self.g.ai.is_step)// if there is ground continue otherwise wait for next move
			ucmd.forwardmove = 0; //0
		else if( AI_CanMove( self, BOT_MOVE_FORWARD))
			ucmd.forwardmove = 100;

		return;
	}


	// Otherwise move slowly, walking wondering what's going on
	if( AI_CanMove( self, BOT_MOVE_FORWARD))
		ucmd.forwardmove = 100;
	else
		ucmd.forwardmove = -100;
}

//==========================================
// BOT_DMclass_CombatMovement
//
// NOTE: Very simple for now, just a basic move about avoidance.
//       Change this routine for more advanced attack movement.
//==========================================
static void BOT_DMclass_CombatMovement(entity &self, usercmd &ucmd)
{
	float	c;
	vector	attackvector;
	float	dist;

	//jalToDo: Convert CombatMovement to a BOT_STATE, allowing
	//it to dodge, but still follow paths, chasing enemy or
	//running away... hmmm... maybe it will need 2 different BOT_STATEs

	if(!self.g.enemy.has_value())
	{
		//do whatever (tmp move wander)
		if( AI_FollowPath(self) )
			BOT_DMclass_Move(self, ucmd);
		return;
	}

	// Randomly choose a movement direction
	c = random();

	if(c < 0.2 && AI_CanMove(self,BOT_MOVE_LEFT))
		ucmd.sidemove -= 400;
	else if(c < 0.4 && AI_CanMove(self,BOT_MOVE_RIGHT))
		ucmd.sidemove += 400;
	else if(c < 0.6 && AI_CanMove(self,BOT_MOVE_FORWARD))
		ucmd.forwardmove += 400;
	else if(c < 0.8 && AI_CanMove(self,BOT_MOVE_BACK))
		ucmd.forwardmove -= 400;

	attackvector = self.s.origin - self.g.enemy->s.origin;
	dist = VectorLength( attackvector);

	if(dist < 75)
		ucmd.forwardmove -= 200;
}

//==========================================
// BOT_DMclass_CheckShot
// Checks if shot is blocked or if too far to shoot
//==========================================
static bool BOT_DMclass_CheckShot(entity &ent, vector point)
{
	vector start, forward, right, offset;

	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);

	offset = { 8, 8, (float)ent.g.viewheight-8 };
	start = P_ProjectSource (ent, ent.s.origin, offset, forward, right);

	//bloqued, don't shoot
	trace tr = gi.trace(start, vec3_origin, vec3_origin, point, ent, MASK_AISOLID);

	if (tr.fraction < 0.3) //just enough to prevent self damage (by now)
		return false;

	return true;
}

//==========================================
// BOT_DMclass_FindEnemy
// Scan for enemy (simplifed for now to just pick any visible enemy)
//==========================================
static bool BOT_DMclass_FindEnemy(entity &self)
{
	entityref	bestenemy = 0;
	float	bestweight = FLT_MAX;
	float	weight;
	vector	dist;

	// we already set up an enemy this frame (reacting to attacks)
	if(self.g.enemy.has_value())
		return true;

	// Find Enemy
	for (const auto &player : entity_range(1, game.maxclients))
	{
		if(player == self || player.solid == SOLID_NOT || (player.g.flags & FL_NOTARGET))
			continue;

		//Ignore players with 0 weight (was set at botstatus)
		if(!self.g.ai.status.playersWeights.contains(player.s.number))
			continue;

		if (!player.g.deadflag && visible(self, player) &&
			gi.inPVS(self.s.origin, player.s.origin))
		{
			//(weight enemies from fusionbot) Is enemy visible, or is it too close to ignore 
			dist = self.s.origin - player.s.origin;
			weight = VectorLength( dist );

			//modify weight based on precomputed player weights
			weight *= (1.0f - self.g.ai.status.playersWeights[player.s.number]);

			if( infront( self, player ) ||
				(weight < 300 ) )
			{
				// Check if best target, or better than current target
				if (weight < bestweight)
				{
					bestweight = weight;
					bestenemy = player;
				}
			}
		}
	}

	// If best enemy, set up
	if(bestenemy.has_value())
	{
		if (AIDevel.debugChased && bot_showcombat)
			gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s as enemy.\n",
			self.client->g.pers.netname.ptr(),
			bestenemy->client->g.pers.netname.ptr());

		self.g.enemy = bestenemy;
		return true;
	}

	return false;	// NO enemy
}

//==========================================
// BOT_DMClass_ChangeWeapon
//==========================================
static bool BOT_DMClass_ChangeWeapon(entity &ent, const gitem_t &item)
{
	// see if we're already using it
	if (item == ent.client->g.pers.weapon)
		return true;

	// Has not picked up weapon yet
	if (!ent.client->g.pers.inventory[item.id])
		return false;

	// Do we have ammo for it?
	if (item.ammo && !ent.client->g.pers.inventory[item.ammo])
		return false;

	// Change to this weapon
	ent.client->g.newweapon = item;
	ent.g.ai.changeweapon_timeout_framenum = level.framenum + (gtime)(6.0 * BASE_FRAMERATE);
	return true;
}

//==========================================
// BOT_DMclass_ChooseWeapon
// Choose weapon based on range & weights
//==========================================
static void BOT_DMclass_ChooseWeapon(entity &self)
{
	float	dist;
	vector	v;
	int		i;
	float	best_weight = 0.0;
	gitem_id best_weapon = ITEM_NONE;
	ai_range weapon_range;

	// if no enemy, then what are we doing here?
	if (!self.g.enemy.has_value())
		return;

	if( self.g.ai.changeweapon_timeout_framenum > level.framenum)
		return;

	// Base weapon selection on distance: 
	v = self.s.origin - self.g.enemy->s.origin;
	dist = VectorLength(v);

	if(dist < 150)
		weapon_range = AIWEAP_MELEE_RANGE;
	else if(dist < 500)	//Medium range limit is Grenade Launcher range
		weapon_range = AIWEAP_SHORT_RANGE;
	else if(dist < 900)
		weapon_range = AIWEAP_MEDIUM_RANGE;
	else 
		weapon_range = AIWEAP_LONG_RANGE;


	for (i = ITEM_WEAPONS_FIRST; i <= ITEM_WEAPONS_LAST; i++)
	{
		if (!AIWeapons[i].weaponItem)
			continue;

		//ignore those we don't have
		if (!self.client->g.pers.inventory[AIWeapons[i].weaponItem] )
			continue;
		
		//ignore those we don't have ammo for
		if (AIWeapons[i].ammoItem	//excepting for those not using ammo
			&& !self.client->g.pers.inventory[AIWeapons[i].ammoItem] )
			continue;
		
		//compare range weights
		if (AIWeapons[i].RangeWeight[weapon_range] > best_weight) {
			best_weight = AIWeapons[i].RangeWeight[weapon_range];
			best_weapon = AIWeapons[i].weaponItem;
		}
	}
	
	//do the change (same weapon, or null best_weapon is covered at ChangeWeapon)
	BOT_DMClass_ChangeWeapon( self, GetItemByIndex(best_weapon));
}

//==========================================
// BOT_DMclass_FireWeapon
// Fire if needed
//==========================================
static void BOT_DMclass_FireWeapon(entity &self, usercmd &ucmd)
{
	//float	c;
	float	firedelay;
	vector  target;
	vector  angles;

	if (!self.g.enemy.has_value() || !self.client->g.pers.weapon.has_value())
		return;

	gitem_id weapon = (gitem_id)(self.client->g.pers.weapon->id - ITEM_WEAPONS_FIRST);
	
	// Aim
	target = self.g.enemy->s.origin;

	// find out our weapon AIM style
	if( AIWeapons[weapon].aimType == AI_AIMSTYLE_PREDICTION_EXPLOSIVE )
	{
		//aim to the feets when enemy isn't higher
		if( self.s.origin[2] + self.g.viewheight > target[2] + (self.g.enemy->mins[2] * 0.8) )
			target[2] += self.g.enemy->mins[2];
	}
	else if ( AIWeapons[weapon].aimType == AI_AIMSTYLE_PREDICTION )
	{
		//jalToDo

	}
	else if ( AIWeapons[weapon].aimType == AI_AIMSTYLE_DROP )
	{
		//jalToDo

	} else { //AI_AIMSTYLE_INSTANTHIT

	}

	// modify attack angles based on accuracy (mess this up to make the bot's aim not so deadly)
	target[0] += (random()-0.5f) * ((MAX_BOT_SKILL - self.g.ai.pers.skillLevel) *2);
	target[1] += (random()-0.5f) * ((MAX_BOT_SKILL - self.g.ai.pers.skillLevel) *2);

	// Set direction
	self.g.ai.move_vector = target - self.s.origin;
	angles = vectoangles (self.g.ai.move_vector);
	self.s.angles = angles;


	// Set the attack 
	firedelay = random()*(MAX_BOT_SKILL*1.8f);
	if (firedelay > (MAX_BOT_SKILL - self.g.ai.pers.skillLevel) && BOT_DMclass_CheckShot(self, target))
		ucmd.buttons = BUTTON_ATTACK;
}

//==========================================
// BOT_DMclass_WeightPlayers
// weight players based on game state
//==========================================
static void BOT_DMclass_WeightPlayers(entity &self)
{
	//clear
	self.g.ai.status.playersWeights.clear();

	for (const auto &player : entity_range(1, game.maxclients))
	{
		if( player == self )
			continue;
		//ignore spectators and dead players
		if( (player.svflags & SVF_NOCLIENT) || player.g.deadflag || !player.inuse )
			continue;

#ifdef CTF
		if( ctf.intVal )
		{
			if( player.client.resp.ctf_team != self.client.resp.ctf_team )
			{
				//being at enemy team gives a small weight, but weight afterall
				self.g.ai.status.playersWeights[i] = 0.2;

				//enemy has redflag
				if( redflag && player.client->g.pers.inventory[redflag->id]
					&& (self.client.resp.ctf_team == CTF_TEAM1) )
				{
					if( !self.client->g.pers.inventory[blueflag->id] ) //don't hunt if you have the other flag, let others do
						self.g.ai.status.playersWeights[i] = 0.9;
				}
				
				//enemy has blueflag
				if( blueflag && player.client->g.pers.inventory[blueflag->id]
					&& (self.client.resp.ctf_team == CTF_TEAM2) )
				{
					if( !self.client->g.pers.inventory[redflag->id] ) //don't hunt if you have the other flag, let others do
						self.g.ai.status.playersWeights[i] = 0.9;
				}
			} 
		}
		else	//if not at ctf every player has some value
#endif
			self.g.ai.status.playersWeights[player.s.number] = 0.3f;
	
	}
}

#ifdef CTF
//==========================================
// BOT_DMclass_WantedFlag
// find needed flag
//==========================================
static gitem_t*(entity self) BOT_DMclass_WantedFlag =
{
	bool hasflag;

	if (!ctf.intVal)
		return 0;
	
	if (!self.client.resp.ctf_team)
		return 0;
	
	//find out if the player has a flag, and what flag is it
	if (redflag && self.client->g.pers.inventory[redflag->id])
		hasflag = true;
	else if (blueflag && self.client->g.pers.inventory[blueflag->id])
		hasflag = true;
	else
		hasflag = false;

	//jalToDo: see if our flag is at base

	if (!hasflag)//if we don't have a flag we want other's team flag
	{
		if (self.client.resp.ctf_team == CTF_TEAM1)
			return blueflag;
		else
			return redflag;
	}
	else	//we have a flag
	{
		if (self.client.resp.ctf_team == CTF_TEAM1)
			return redflag;
		else
			return blueflag;
	}

	return 0;
}
#endif

//==========================================
// BOT_DMclass_WeightInventory
// weight items up or down based on bot needs
//==========================================
static void BOT_DMclass_WeightInventory(entity &self)
{
	float		LowNeedFactor = 0.5;
	int			i;

	//reset with persistant values
	self.g.ai.status.inventoryWeights = self.g.ai.pers.inventoryWeights;
	

	//weight ammo down if bot doesn't have the weapon for it,
	//or denny weight for it, if bot is packed up.
	//------------------------------------------------------

	//AMMO_BULLETS

	if (!AI_CanPick_Ammo (self, GetItemByIndex(AIWeapons[ITEM_MACHINEGUN].ammoItem)) )
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_MACHINEGUN].ammoItem] = 0.0;
	//find out if it has a weapon for this amno
	else if (!self.client->g.pers.inventory[AIWeapons[ITEM_CHAINGUN].weaponItem]
		&& !self.client->g.pers.inventory[AIWeapons[ITEM_MACHINEGUN].weaponItem] )
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_MACHINEGUN].ammoItem] *= LowNeedFactor;

	//AMMO_SHELLS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, GetItemByIndex(AIWeapons[ITEM_SHOTGUN].ammoItem)) )
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_SHOTGUN].ammoItem] = 0.0;
	//find out if it has a weapon for this amno
	else if (!self.client->g.pers.inventory[AIWeapons[ITEM_SHOTGUN].weaponItem]
		&& !self.client->g.pers.inventory[AIWeapons[ITEM_SUPER_SHOTGUN].weaponItem] )
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_SHOTGUN].ammoItem] *= LowNeedFactor;

	//AMMO_ROCKETS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, GetItemByIndex(AIWeapons[ITEM_ROCKET_LAUNCHER].ammoItem)))
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_ROCKET_LAUNCHER].ammoItem] = 0.0;
	//find out if it has a weapon for this amno
	else if (!self.client->g.pers.inventory[AIWeapons[ITEM_ROCKET_LAUNCHER].weaponItem] )
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_ROCKET_LAUNCHER].ammoItem] *= LowNeedFactor;

	//AMMO_GRENADES: 

	//find if it's packed up
	if (!AI_CanPick_Ammo (self, GetItemByIndex(AIWeapons[ITEM_GRENADES].ammoItem)))
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_GRENADES].ammoItem] = 0.0;
	//grenades are also weapons, and are weighted down by LowNeedFactor in weapons group
	
	//AMMO_CELLS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, GetItemByIndex(AIWeapons[ITEM_HYPERBLASTER].ammoItem)))
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_HYPERBLASTER].ammoItem] = 0.0;
	//find out if it has a weapon for this amno
	else if (!self.client->g.pers.inventory[AIWeapons[ITEM_HYPERBLASTER].weaponItem]
		&& !self.client->g.pers.inventory[AIWeapons[ITEM_BFG].weaponItem]
		&& !self.client->g.pers.inventory[ITEM_POWER_SHIELD]
		&& !self.client->g.pers.inventory[ITEM_POWER_SCREEN])
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_HYPERBLASTER].ammoItem] *= LowNeedFactor;

	//AMMO_SLUGS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, GetItemByIndex(AIWeapons[ITEM_RAILGUN].ammoItem)))
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_RAILGUN].ammoItem] = 0.0;
	//find out if it has a weapon for this amno
	else if (!self.client->g.pers.inventory[AIWeapons[ITEM_RAILGUN].weaponItem] )
		self.g.ai.status.inventoryWeights[AIWeapons[ITEM_RAILGUN].ammoItem] *= LowNeedFactor;


	//WEAPONS
	//-----------------------------------------------------

	//weight weapon down if bot already has it
	for (i=ITEM_WEAPONS_FIRST; i<=ITEM_WEAPONS_LAST; i++)
		if ( AIWeapons[i].weaponItem && self.client->g.pers.inventory[AIWeapons[i].weaponItem])
			self.g.ai.status.inventoryWeights[AIWeapons[i].weaponItem] *= LowNeedFactor;

	//ARMOR
	//-----------------------------------------------------
	if (!AI_CanUseArmor (GetItemByIndex(ITEM_ARMOR_JACKET), self ))
		self.g.ai.status.inventoryWeights[ITEM_ARMOR_JACKET] = 0.0;

	if (!AI_CanUseArmor ( GetItemByIndex(ITEM_ARMOR_COMBAT), self ))
		self.g.ai.status.inventoryWeights[ITEM_ARMOR_COMBAT] = 0.0;

	if (!AI_CanUseArmor ( GetItemByIndex(ITEM_ARMOR_BODY), self ))
		self.g.ai.status.inventoryWeights[ITEM_ARMOR_BODY] = 0.0;

#ifdef CTF
	//TECH :
	//-----------------------------------------------------
	if ( self.client->pers.inventory[ITEM_TECH1] 
		|| self.client->pers.inventory[ITEM_TECH2] 
		|| self.client->pers.inventory[ITEM_TECH3] 
		|| self.client->pers.inventory[ITEM_TECH4] )
	{
		self.g.ai.status.inventoryWeights[ITEM_TECH1] = 0.0; 
		self.g.ai.status.inventoryWeights[ITEM_TECH2] = 0.0; 
		self.g.ai.status.inventoryWeights[ITEM_TECH3] = 0.0;
		self.g.ai.status.inventoryWeights[ITEM_TECH4] = 0.0;
	}

	//CTF: 
	//-----------------------------------------------------
	if( ctf.intVal)
	{
		gitem_t		*wantedFlag = BOT_DMclass_WantedFlag( self ); //Returns the flag gitem_t
		
		//flags have weights defined inside persistant inventory. Remove weight from the unwanted one/s.
		if (blueflag && blueflag != wantedFlag)
			self.g.ai.status.inventoryWeights[blueflag->id] = 0.0;
		else if (redflag && redflag != wantedFlag)
			self.g.ai.status.inventoryWeights[redflag->id] = 0.0;
	}
#endif
}

//==========================================
// BOT_DMclass_UpdateStatus
// update ai.status values based on bot state,
// so ai can decide based on these settings
//==========================================
static void BOT_DMclass_UpdateStatus( entity &self )
{
	self.g.enemy = self.g.movetarget = 0;

	// Set up for new client movement: jalfixme
	self.s.angles = self.client->ps.viewangles;
	self.client->ps.pmove.set_delta_angles(vec3_origin);

	if (self.client->ps.pmove.pm_flags & PMF_TIME_TELEPORT)
		self.g.ai.status.TeleportReached = true;
	else
		self.g.ai.status.TeleportReached = false;

	//set up AI status for the upcoming AI_frame
	BOT_DMclass_WeightInventory( self );	//weight items
	BOT_DMclass_WeightPlayers( self );		//weight players
}

//==========================================
// BOT_DMClass_BloquedTimeout
// the bot has been bloqued for too long
//==========================================
static void BOT_DMClass_BloquedTimeout(entity &self)
{
	self.g.health = 0;
	self.g.ai.bloqued_timeout_framenum = level.framenum + (gtime)(15.0 * BASE_FRAMERATE);
	self.g.die(self, self, self, 100000, vec3_origin);
	self.g.nextthink = level.framenum + 1;
}

//==========================================
// BOT_DMclass_DeadFrame
// ent is dead = run this think func
//==========================================
static void BOT_DMclass_DeadFrame(entity &self)
{
	usercmd	ucmd = {};

	// ask for respawn
	self.client->g.buttons = BUTTON_NONE;
	ucmd.buttons = BUTTON_ATTACK;
	ClientThink (self, ucmd);
	self.g.nextthink = level.framenum + 1;
}

//==========================================
// BOT_DMclass_RunFrame
// States Machine & call client movement
//==========================================
static void BOT_DMclass_RunFrame( entity &self )
{
	usercmd	ucmd = {};

	// Look for enemies
	if( BOT_DMclass_FindEnemy(self) )
	{
		BOT_DMclass_ChooseWeapon( self );
		BOT_DMclass_FireWeapon( self, ucmd );
		self.g.ai.state = BOT_STATE_ATTACK;
		self.g.ai.state_combat_timeout_framenum = level.framenum + (gtime)(1.0 * BASE_FRAMERATE);
	
	} else if( self.g.ai.state == BOT_STATE_ATTACK && 
		level.framenum > self.g.ai.state_combat_timeout_framenum)
	{
		//Jalfixme: change to: AI_SetUpStateMove(self);
		self.g.ai.state = BOT_STATE_WANDER;
	}

	// Execute the move, or wander
	if( self.g.ai.state == BOT_STATE_MOVE )
		BOT_DMclass_Move( self, ucmd );

	else if(self.g.ai.state == BOT_STATE_ATTACK)
		BOT_DMclass_CombatMovement( self, ucmd );

	else if ( self.g.ai.state == BOT_STATE_WANDER )
		BOT_DMclass_Wander( self, ucmd );


	//set up for pmove
	ucmd.set_angles(self.s.angles);

	// set approximate ping and show values
	ucmd.msec = 100;
	self.client->ping = ucmd.msec;

	// send command through id's code
	ClientThink( self, ucmd );
	self.g.nextthink = level.framenum + 1;
}

//==========================================
// BOT_DMclass_InitPersistant
// Persistant after respawns. 
//==========================================
void BOT_DMclass_InitPersistant(entity &self)
{
	//set 'class' functions
	self.g.ai.pers.RunFrame = BOT_DMclass_RunFrame;
	self.g.ai.pers.UpdateStatus = BOT_DMclass_UpdateStatus;
	self.g.ai.pers.bloquedTimeout = BOT_DMClass_BloquedTimeout;
	self.g.ai.pers.deadFrame = BOT_DMclass_DeadFrame;

	//available moveTypes for this class
	self.g.ai.pers.moveTypesMask = (LINK_MOVE|LINK_STAIRS|LINK_FALL|LINK_WATER|LINK_WATERJUMP|LINK_JUMPPAD|LINK_PLATFORM|LINK_TELEPORT|LINK_LADDER|LINK_JUMP|LINK_CROUCH);

	//Persistant Inventory Weights (0 = can not pick)
	self.g.ai.pers.inventoryWeights = {};

	//weapons
	self.g.ai.pers.inventoryWeights[ITEM_SHOTGUN] = 0.5f;
	self.g.ai.pers.inventoryWeights[ITEM_SUPER_SHOTGUN] = 0.7f;
	self.g.ai.pers.inventoryWeights[ITEM_MACHINEGUN] = 0.5f;
	self.g.ai.pers.inventoryWeights[ITEM_CHAINGUN] = 0.7f;
	self.g.ai.pers.inventoryWeights[ITEM_GRENADE_LAUNCHER] = 0.6f;
	self.g.ai.pers.inventoryWeights[ITEM_ROCKET_LAUNCHER] = 0.8f;
	self.g.ai.pers.inventoryWeights[ITEM_HYPERBLASTER] = 0.7f;
	self.g.ai.pers.inventoryWeights[ITEM_RAILGUN] = 0.8f;
	self.g.ai.pers.inventoryWeights[ITEM_BFG] = 0.5f;

	//ammo
	self.g.ai.pers.inventoryWeights[ITEM_SHELLS] = 0.5f;
	self.g.ai.pers.inventoryWeights[ITEM_BULLETS] = 0.5f;
	self.g.ai.pers.inventoryWeights[ITEM_CELLS] = 0.5f;
	self.g.ai.pers.inventoryWeights[ITEM_ROCKETS] = 0.5f;
	self.g.ai.pers.inventoryWeights[ITEM_SLUGS] = 0.5f;
	self.g.ai.pers.inventoryWeights[ITEM_GRENADES] = 0.5f;
	
	//armor
	self.g.ai.pers.inventoryWeights[ITEM_ARMOR_BODY] = 0.9f;
	self.g.ai.pers.inventoryWeights[ITEM_ARMOR_COMBAT] = 0.8f;
	self.g.ai.pers.inventoryWeights[ITEM_ARMOR_JACKET] = 0.5f;
	self.g.ai.pers.inventoryWeights[ITEM_ARMOR_SHARD] = 0.2f;
	
#ifdef CTF
	//techs
	self.g.ai.pers.inventoryWeights[ITEM_TECH1] = 0.5;
	self.g.ai.pers.inventoryWeights[ITEM_TECH2] = 0.5;
	self.g.ai.pers.inventoryWeights[ITEM_TECH3] = 0.5;
	self.g.ai.pers.inventoryWeights[ITEM_TECH4] = 0.5;

	// flags
	if(ctf.intVal)
	{
		self.g.ai.pers.inventoryWeights[ITEM_FLAG1] = 3.0;
		self.g.ai.pers.inventoryWeights[ITEM_FLAG2] = 3.0;
	}
#endif
}


#endif