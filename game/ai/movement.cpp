import usercmd;

#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "../../lib/entity.h"
#include "../util.h"
#include "movement.h"

//==========================================
// AI_CanMove
// Checks if bot can move (really just checking the ground)
// Also, this is not a real accurate check, but does a
// pretty good job and looks for lava/slime.  
//==========================================
bool AI_CanMove(entity &self, ai_direction_t direction)
{
	vector forward, right;
	vector offset,start,end;
	vector angles;

	// Now check to see if move will move us off an edge
	angles = self.s.angles;

	if(direction == BOT_MOVE_LEFT)
		angles[1] += 90;
	else if(direction == BOT_MOVE_RIGHT)
		angles[1] -= 90;
	else if(direction == BOT_MOVE_BACK)
		angles[1] -=180;

	// Set up the vectors
	AngleVectors (angles, &forward, &right, nullptr);

	offset = { 36, 0, 24 };
	start = G_ProjectSource (self.s.origin, offset, forward, right);

	offset = { 36, 0, -100 };
	end = G_ProjectSource (self.s.origin, offset, forward, right);

	trace tr = gi.traceline(start, end, self, MASK_AISOLID );
	return tr.fraction != 1.0 && !(tr.contents & (CONTENTS_LAVA|CONTENTS_SLIME));// yup, can move
}


//===================
//  AI_IsStep
//  Checks the floor one step below the player. Used to detect
//  if the player is really falling or just walking down a stair.
//===================
bool AI_IsStep(entity &ent)
{
	vector	point;
	
	//determine a point below
	point[0] = ent.s.origin[0];
	point[1] = ent.s.origin[1];
	point[2] = ent.s.origin[2] - (1.6f*AI_STEPSIZE);
	
	//trace to point
	trace tr = gi.trace(ent.s.origin, ent.mins, ent.maxs, point, ent, MASK_PLAYERSOLID);
	
	if (tr.normal[2] < 0.7 && !tr.startsolid)
		return false;
	
	//found solid.
	return true;
}


//==========================================
// AI_IsLadder
// check if entity is touching in front of a ladder
//==========================================
bool AI_IsLadder(vector origin, vector v_angle, vector mins, vector maxs, entityref passent)
{
	vector spot;
	vector flatforward, zforward;

	AngleVectors( v_angle, &zforward, nullptr, nullptr);

	// check for ladder
	flatforward[0] = zforward[0];
	flatforward[1] = zforward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	spot = origin + flatforward;

	trace tr = gi.trace(origin, mins, maxs, spot, passent, MASK_AISOLID);
	return tr.fraction < 1 && (tr.contents & CONTENTS_LADDER);
}


//==========================================
// AI_CheckEyes
// Helper for ACEMV_SpecialMove. 
// Tries to turn when in front of obstacle
//==========================================
inline bool AI_CheckEyes(entity &self, usercmd &ucmd)
{
	vector  forward, right;
	vector  leftstart, rightstart,focalpoint;
	vector  dir,offset;
	trace traceRight;
	trace traceLeft;

	// Get current angle and set up "eyes"
	dir = self.s.angles;
	AngleVectors (dir, &forward, &right, nullptr);

	if(!self.g.movetarget.has_value())
		offset = { 200,0,self.maxs[2]*0.5f }; // focalpoint
	else
		offset = { 64,0,self.maxs[2]*0.5f }; // wander focalpoint 
	
	focalpoint = G_ProjectSource (self.s.origin, offset, forward, right);

	offset = { 0, 18, self.maxs[2]*0.5f };
	leftstart = G_ProjectSource (self.s.origin, offset, forward, right);
	offset[1] -= 36; //VectorSet(offset, 0, -18, self->maxs[2]*0.5);
	rightstart = G_ProjectSource (self.s.origin, offset, forward, right);

	traceRight = gi.traceline(rightstart, focalpoint, self, MASK_AISOLID);
	traceLeft = gi.traceline(leftstart, focalpoint, self, MASK_AISOLID);

	// Find the side with more open space and turn
	if(traceRight.fraction != 1 || traceLeft.fraction != 1)
	{
		if(traceRight.fraction > traceLeft.fraction)
			self.s.angles[YAW] += (1.0f - traceLeft.fraction) * 45.0f;
		else
			self.s.angles[YAW] += -(1.0f - traceRight.fraction) * 45.0f;
		
		ucmd.forwardmove = 400;
		return true;
	}
				
	return false;
}

//==========================================
// AI_SpecialMove
// Handle special cases of crouch/jump
// If the move is resolved here, this function returns true.
//==========================================
bool AI_SpecialMove(entity &self, usercmd &ucmd)
{
	vector forward;
	vector	boxmins, boxmaxs, boxorigin;

	// Get current direction
	AngleVectors({ 0, self.s.angles[YAW], 0 }, &forward, nullptr, nullptr);

	//make sure we are bloqued
	boxorigin = self.s.origin + (8 * forward); //move box by 8 to front
	trace tr = gi.trace(self.s.origin, self.mins, self.maxs, boxorigin, self, MASK_AISOLID);
	if( !tr.startsolid && tr.fraction == 1.0 ) // not bloqued
		return false;

	if( (self.g.ai.pers.moveTypesMask & LINK_CROUCH) || self.g.ai.is_swim )
	{
		//crouch box
		boxorigin = self.s.origin;
		boxmins = self.mins;
		boxmaxs = self.maxs;
		boxmaxs[2] = 14;	//crouched size
		boxorigin += 8 * forward; //move box by 8 to front
		//see if bloqued
		tr = gi.trace(boxorigin, boxmins, boxmaxs, boxorigin, self, MASK_AISOLID);
		if( !tr.startsolid ) // can move by crouching
		{
			ucmd.forwardmove = 400;
			ucmd.upmove = -400;
			return true;
		}
	}

	if( (self.g.ai.pers.moveTypesMask & LINK_JUMP) && self.g.groundentity.has_value())
	{
		//jump box
		boxorigin = self.s.origin;
		boxmins = self.mins;
		boxmaxs = self.maxs;
		boxorigin += 8 * forward;	//move box by 8 to front
		//
		boxorigin[2] += ( boxmins[2] + AI_JUMPABLE_HEIGHT );	//put at bottom + jumpable height
		boxmaxs[2] = boxmaxs[2] - boxmins[2];	//total player box height in boxmaxs
		boxmins[2] = 0;
		if( boxmaxs[2] > AI_JUMPABLE_HEIGHT ) //the player is smaller than AI_JUMPABLE_HEIGHT
		{
			boxmaxs[2] -= AI_JUMPABLE_HEIGHT;
			tr = gi.trace(boxorigin, boxmins, boxmaxs, boxorigin, self, MASK_AISOLID);
			if( !tr.startsolid )	//can move by jumping
			{
				ucmd.forwardmove = 400;
				ucmd.upmove = 400;
				
				return true;
			}
		}
	}

	//nothing worked, check for turning
	return AI_CheckEyes(self, ucmd);
}

//==========================================
// AI_ChangeAngle
// Make the change in angles a little more gradual, not so snappy
// Subtle, but noticeable.
// 
// Modified from the original id ChangeYaw code...
//==========================================
void AI_ChangeAngle(entity &ent)
{
	float	ideal_yaw;
	float   ideal_pitch;
	float	current_yaw;
	float   current_pitch;
	float	move;
	float	speed;
	vector  ideal_angle;

	// Normalize the move angle first
	VectorNormalize(ent.g.ai.move_vector);

	current_yaw = anglemod(ent.s.angles[YAW]);
	current_pitch = anglemod(ent.s.angles[PITCH]);

	ideal_angle = vectoangles (ent.g.ai.move_vector);

	ideal_yaw = anglemod(ideal_angle[YAW]);
	ideal_pitch = anglemod(ideal_angle[PITCH]);

	// Yaw
	if (current_yaw != ideal_yaw)
	{
		move = ideal_yaw - current_yaw;
		speed = ent.g.yaw_speed;
		if (ideal_yaw > current_yaw)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent.s.angles[YAW] = anglemod (current_yaw + move);
	}


	// Pitch
	if (current_pitch != ideal_pitch)
	{
		move = ideal_pitch - current_pitch;
		speed = ent.g.yaw_speed;
		if (ideal_pitch > current_pitch)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent.s.angles[PITCH] = anglemod (current_pitch + move);
	}
}

//==========================================
// AI_MoveToGoalEntity
// Set bot to move to it's movetarget. Short range goals
//==========================================
bool AI_MoveToGoalEntity(entity &self, usercmd &ucmd)
{
	if (!self.g.movetarget.has_value())
		return false;

	// If a rocket or grenade is around deal with it
	// Simple, but effective (could be rewritten to be more accurate)
	if(self.g.movetarget->g.type == ET_ROCKET ||
	   self.g.movetarget->g.type == ET_GRENADE ||
	   self.g.movetarget->g.type == ET_HANDGRENADE)
	{
		self.g.ai.move_vector = self.g.movetarget->s.origin - self.s.origin;
		AI_ChangeAngle(self);

		if(AIDevel.debugChased && bot_showcombat)
			gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: Oh crap a rocket!\n", self.client->g.pers.netname.ptr());

		// strafe left/right
		if(Q_rand()%1 && AI_CanMove(self, BOT_MOVE_LEFT))
			ucmd.sidemove = -400;
		else if(AI_CanMove(self, BOT_MOVE_RIGHT))
			ucmd.sidemove = 400;
		return true;
	}

	// Set bot's movement direction
	self.g.ai.move_vector = self.g.movetarget->s.origin - self.s.origin;
	AI_ChangeAngle(self);
	if(!AI_CanMove(self, BOT_MOVE_FORWARD) ) 
	{
		self.g.movetarget = 0;
		ucmd.forwardmove = -100;
		return false;
	}

	//Move
	ucmd.forwardmove = 400;
	return true;
}

#endif