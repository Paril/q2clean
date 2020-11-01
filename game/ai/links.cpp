#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "ai.h"
#include "navigation.h"

//==========================================
// AI_VisibleOrigins
// same as is visible, but doesn't need ents
//==========================================
inline bool AI_VisibleOrigins(vector spot1, vector spot2)
{
	trace tr = gi.traceline(spot1, spot2, 0, MASK_NODESOLID);
	return tr.fraction == 1.0 && !tr.startsolid;
}

//=================
//AI_findNodeInRadius
//
// Copy of findradius to act with nodes instead of entities
// Setting up ignoreHeight uses a cilinder instead of a sphere (used to catch fall links)
//=================
node_id AI_findNodeInRadius(node_id from, vector org, float rad, bool ignoreHeight)
{
	const size_t numNodes = nav.nodes.size();

	if( from < 0 )
		return NODE_INVALID;
	else if( from > numNodes )
		return NODE_INVALID;
	else if( !numNodes )
		return NODE_INVALID;
	else
		from++;

	for( ; from < numNodes; from++)
	{
		const nav_node &node = nav.nodes[from];
		vector eorg = org - node.origin;
		
		if (ignoreHeight)
			eorg[2] = 0;
		
		if (VectorLength(eorg) > rad)
			continue;

		return from;
	}

	return NODE_INVALID;
}

//==========================================
// AI_FindLinkDistance
// returns world distance between both nodes
//==========================================
float AI_FindLinkDistance(node_id n1, node_id n2)
{
	nav_node &pn1 = nav.nodes[n1], &pn2 = nav.nodes[n2];

	// Teleporters exception: JALFIXME: add check for Destiny being teleporter's target
	if ((pn1.flags & NODEFLAGS_TELEPORTER_IN) && (pn2.flags & NODEFLAGS_TELEPORTER_OUT))
		return NODE_DENSITY; //not 0, just because teleporting has a strategical cost

	return VectorDistance(pn1.origin, pn2.origin);
}

//==========================================
// AI_PlinkExists
// Just check if the plink is already stored
//==========================================
bool AI_PlinkExists(node_id n1, node_id n2)
{
	if (n1 == n2)
		return false;

	for (const auto &link : nav.nodes[n1].links)
		if (link.node == n2)
			return true;

	return false;
}

//==========================================
// AI_AddLink
// force-add of a link
//==========================================
bool AI_AddLink(node_id n1, node_id n2, ai_link_type linkType)
{
	//never store self-link
	if (n1 == n2)
		return false;
	
	//already referenced
	if(AI_PlinkExists(n1, n2))
		return false;

	if (linkType == LINK_INVALID)
		return false;

	nav_node &pn1 = nav.nodes[n1];

	//add the link
	pn1.links.push_back({
		n2,
		(uint32_t)AI_FindLinkDistance(n1, n2),
		linkType
	});
	return true;
}

//==========================================
// AI_PlinkMoveType
// return moveType of an already stored link
//==========================================
ai_link_type AI_PlinkMoveType(node_id n1, node_id n2)
{
	if (!nav.loaded)
		return LINK_INVALID;

	if (n1 == n2)
		return LINK_INVALID;

	for (const auto &link : nav.nodes[n1].links)
		if (link.node == n2 )
			return link.moveType;

	return LINK_INVALID;
}

//==========================================
// AI_IsWaterJumpLink
// check against the world if we can link it
//==========================================
constexpr int AI_WATERJUMP_HEIGHT = 24;

static ai_link_type AI_IsWaterJumpLink(node_id n1, node_id n2)
{
	vector	waterorigin;
	vector	solidorigin;
	trace	tr;
	float	heightdiff;
	
	nav_node &pn1 = nav.nodes[n1];
	nav_node &pn2 = nav.nodes[n2];

	//find n2 floor
	tr = gi.trace(pn2.origin, { -15, -15, 0 }, { 15, 15, 0 }, pn2.origin - vector { 0, 0, AI_JUMPABLE_HEIGHT }, 0, MASK_NODESOLID);
	if( tr.startsolid || tr.fraction == 1.0 )
		return LINK_INVALID;
	
	solidorigin = tr.endpos;
	
	if( gi.pointcontents(pn1.origin) & MASK_WATER )
		waterorigin = pn1.origin;
	else
		//try finding water below, isn't it a node_water?
		return LINK_INVALID;

	if ( solidorigin[2] >= waterorigin[2])
		heightdiff = solidorigin[2] - waterorigin[2];
	else
		heightdiff = waterorigin[2] - solidorigin[2];

	if( heightdiff > AI_WATERJUMP_HEIGHT )	//jalfixme: find true waterjump size
		return LINK_INVALID;

	//now find if bloqued
	waterorigin[2] = pn2.origin[2];
	tr = gi.trace(pn1.origin, { -15, -15, 0 }, { 15, 15, 0 }, waterorigin, 0, MASK_NODESOLID );
	if( tr.fraction < 1.0 )
		return LINK_INVALID;

	tr = gi.trace(waterorigin, { -15, -15, 0 }, { 15, 15, 0 }, pn2.origin, 0, MASK_NODESOLID );
	if( tr.fraction < 1.0 )
		return LINK_INVALID;

	return LINK_WATERJUMP;
}

//==========================================
// AI_GravityBoxStep
// move the box one step for walk movetype
//==========================================
static ai_link_type AI_GravityBoxStep(vector origin, float scale, vector destvec, vector &neworigin, vector mins, vector maxs)
{
	trace	tr;
	vector	v1, v2, forward, up, angles, movedir;
	ai_link_type		movemask = LINK_NONE;
	int		eternalfall = 0;
	float	xzdist, xzscale;
	float	ydist, yscale;
	float	dist;

	tr = gi.trace(origin, mins, maxs, origin, 0, MASK_NODESOLID );
	if( tr.startsolid )
		return LINK_INVALID;
	
	movedir = destvec - origin;
	VectorNormalize( movedir );
	angles = vectoangles (movedir);
	
	//remaining distance in planes
	if( scale < 1 )
		scale = 1;

	dist = VectorDistance( origin, destvec );
	if( scale > dist )
		scale = dist;
	
	xzscale = scale;
	xzdist = VectorDistance({ origin[0], origin[1], destvec[2] }, destvec );
	if( xzscale > xzdist )
		xzscale = xzdist;
	
	yscale = scale;
	ydist = VectorDistance({ 0,0,origin[2] }, { 0,0,destvec[2] });
	if( yscale > ydist )
		yscale = ydist;
	
	//float move step
	if( gi.pointcontents( origin ) & MASK_WATER )
	{
		angles[ROLL] = 0;
		AngleVectors( angles, &forward, nullptr, &up );
		
		neworigin = origin + (scale * movedir);
		tr = gi.trace(origin, mins, maxs, neworigin, 0, MASK_NODESOLID);
		if( tr.startsolid || tr.fraction < 1.0 ) 
			neworigin = origin;	//update if valid
		
		if(origin == neworigin)
			return LINK_INVALID;
		
		if( gi.pointcontents( neworigin ) & MASK_WATER )
			return LINK_WATER;
		
		//jal: Actually GravityBox can't leave water.
		//return INVALID and WATERJUMP, so it can validate the rest outside
		return (LINK_INVALID|LINK_WATERJUMP);
	}

	//gravity step

	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AngleVectors( angles, &forward, nullptr, nullptr);
	VectorNormalize( forward );

	// try moving forward
	neworigin = origin + (xzscale * forward);
	tr = gi.trace(origin, mins, maxs, neworigin, 0, MASK_NODESOLID );
	if( tr.fraction == 1.0 ) //moved
	{	
		movemask |= LINK_MOVE;
		goto droptofloor;
	}
	else
	{	//bloqued, try stepping up
		v1 = origin;
		v2 = v1 + (xzscale * forward);
		for( ; v1[2] < origin[2] + AI_JUMPABLE_HEIGHT; v1[2] += scale, v2[2] += scale )
		{
			tr = gi.trace(v1, mins, maxs, v2, 0, MASK_NODESOLID );
			if( !tr.startsolid && tr.fraction == 1.0 )
			{
				neworigin = v2;
				if( origin[2] + AI_STEPSIZE > v2[2] )
					movemask |= LINK_STAIRS;
				else
					movemask |= LINK_JUMP;

				goto droptofloor;
			}
		}

		//still failed, try slide move
		neworigin = origin + (xzscale * forward);
		tr = gi.trace(origin, mins, maxs, neworigin, 0, MASK_NODESOLID );
		if( tr.normal[2] < 0.5 && tr.normal[2] >= -0.4 )
		{
			neworigin = tr.endpos;
			v1 = tr.normal;
			v1[2] = 0;
			VectorNormalize( v1 );
			neworigin += xzscale * v1;
			
			//if new position is closer to destiny, might be valid
			if( VectorDistance( origin, destvec ) > VectorDistance( neworigin, destvec ) )
			{
				tr = gi.trace(tr.endpos, mins, maxs, neworigin, 0, MASK_NODESOLID );
				if( !tr.startsolid && tr.fraction == 1.0 )
					goto droptofloor;
			}
		}

		neworigin = origin;
		return LINK_INVALID;
	}

droptofloor:

	while(eternalfall < 20000000) 
	{
		if( gi.pointcontents(neworigin) & MASK_WATER ) {

			if( origin[2] > neworigin[2] + AI_JUMPABLE_HEIGHT )
				movemask |= LINK_FALL;
			else if ( origin[2] > neworigin[2] + AI_STEPSIZE )
				movemask |= LINK_STAIRS;

			return movemask;
		}

		tr = gi.trace(neworigin, mins, maxs, { neworigin[0], neworigin[1], (neworigin[2] - AI_STEPSIZE) }, 0, MASK_NODESOLID );
		if( tr.startsolid )
			return LINK_INVALID;

		neworigin = tr.endpos;
		if( tr.fraction < 1.0 )
		{
			if( origin[2] > neworigin[2] + AI_JUMPABLE_HEIGHT )
				movemask |= LINK_FALL;
			else if ( origin[2] > neworigin[2] + AI_STEPSIZE )
				movemask |= LINK_STAIRS;

			if(origin == neworigin)
				return LINK_INVALID;

			return movemask;
		}

		eternalfall++;
	}

	gi.error ("ETERNAL FALL\n");
}

//==========================================
// AI_RunGravityBox
// move a box along the link
//==========================================
static ai_link_type AI_RunGravityBox( node_id n1, node_id n2 )
{
	ai_link_type	move;
	ai_link_type	movemask = LINK_NONE;
	float		movescale = 8;
	trace		tr;
	vector		boxmins, boxmaxs;
	vector		o1;
	vector		v1;
	int			eternalcount = 0;

	if( n1 == n2 )
		return LINK_INVALID;

	//set up box
	boxmins = { -15, -15, -24 };
	boxmaxs = { 15, 15, 32 };

	//try some shortcuts before
	nav_node &pn1 = nav.nodes[n1];
	nav_node &pn2 = nav.nodes[n2];

	//water link shortcut
	if( gi.pointcontents(pn1.origin) & MASK_WATER &&
		gi.pointcontents(pn2.origin) & MASK_WATER &&
		AI_VisibleOrigins(pn1.origin, pn2.origin) )
		return LINK_WATER;
	//waterjump link
	if( gi.pointcontents(pn1.origin) & MASK_WATER &&
		!(gi.pointcontents(pn2.origin) & MASK_WATER) &&
		VectorDistance(pn1.origin, pn2.origin) < NODE_DENSITY )
	{
		if( AI_IsWaterJumpLink( n1, n2 ) == LINK_WATERJUMP )
			return LINK_WATERJUMP;
	}

	//proceed

	//put box at first node
	o1 = pn1.origin;
	tr = gi.trace(o1, boxmins, boxmaxs, o1, 0, MASK_NODESOLID );
	if( tr.startsolid )
	{
		//try crouched
		boxmaxs[2] = 14;
		tr = gi.trace(o1, boxmins, boxmaxs, o1, 0, MASK_NODESOLID );
		if( tr.startsolid )
			return LINK_INVALID;

		//crouched = true;
		movemask |= LINK_CROUCH;
	}

	//start moving the box to o2
	while(eternalcount < 20000000)
	{
		move = AI_GravityBoxStep( o1, movescale, pn2.origin, v1, boxmins, boxmaxs);
		if( move & LINK_INVALID && !(movemask & LINK_CROUCH)/*!crouched*/ )
		{
			//retry crouched
			boxmaxs[2] = 14;
			//crouched = true;
			movemask |= LINK_CROUCH;
			move = AI_GravityBoxStep( o1, movescale, pn2.origin, v1, boxmins, boxmaxs);
		}

		if( move & LINK_INVALID )
		{
			//gravitybox can't reach waterjump links. So, check them here
			if( move & LINK_WATERJUMP )
				if( AI_IsWaterJumpLink( n1, n2 ) == LINK_WATERJUMP )
					return LINK_WATERJUMP;
			return ( movemask|move );
		}
		
		//next
		movemask |= move;
		o1 = v1;

		//check for reached
		if( VectorDistance( o1, pn2.origin ) < 24 && AI_VisibleOrigins( o1, pn2.origin ) )
		{
			movemask |= move;
			return movemask;
		}

		eternalcount++;
	}

	gi.error ("ETERNAL COUNT\n"); //should never get here
}

//==========================================
// AI_GravityBoxToLink
// move a box along the link
//==========================================
ai_link_type AI_GravityBoxToLink(node_id n1, node_id n2)
{
	//find out the link move type against the world
	ai_link_type link = AI_RunGravityBox( n1, n2 );
	
	nav_node &pn2 = nav.nodes[n2];

	//don't fall to JUMPAD nodes, or will be sent up again
	if((pn2.flags & NODEFLAGS_JUMPPAD) && (link & LINK_FALL))
		return LINK_INVALID;

	//simplify
	if( link & LINK_INVALID )
		return LINK_INVALID;

	if( link & LINK_CLIMB )
		return LINK_INVALID; // No actual bot is able to climb

	if( link & LINK_WATERJUMP )
		return LINK_WATERJUMP;

	if( link == LINK_WATER || link == (LINK_WATER|LINK_CROUCH) )	//only pure flags
		return LINK_WATER;

	if( link & LINK_CROUCH )
		return LINK_CROUCH;

	if( link & LINK_JUMP )
		return LINK_JUMP;	//there are simple ledge jumps only

	if( link & LINK_FALL )
		return LINK_FALL;

	if( link & LINK_STAIRS )
		return LINK_STAIRS;

	return LINK_MOVE;
}


//==========================================
// AI_FindFallOrigin
// helper for AI_IsJumpLink
//==========================================
static ai_link_type AI_FindFallOrigin(node_id n1, node_id n2, vector &fallorigin)
{
	ai_link_type			move;
	ai_link_type			movemask = LINK_NONE;
	float		movescale = 8;
	trace		tr;
	vector		boxmins, boxmaxs;
	vector		o1;
	vector		v1;
	int			eternalcount = 0;

	if( n1 == n2 )
		return LINK_INVALID;

	//set up box
	boxmins = { -15, -15, -24 };
	boxmaxs = { 15, 15, 32 };
	
	nav_node &pn1 = nav.nodes[n1];

	//put box at first node
	o1 = pn1.origin;
	tr = gi.trace(o1, boxmins, boxmaxs, o1, 0, MASK_NODESOLID );
	if( tr.startsolid )
		return LINK_INVALID;

	nav_node &pn2 = nav.nodes[n2];

	//moving the box to o2 until falls. Keep last origin before falling
	while(1)
	{
		move = AI_GravityBoxStep( o1, movescale, pn2.origin, v1, boxmins, boxmaxs);

		if( move & LINK_INVALID )
			return LINK_INVALID;
		
		movemask |= move;

		if( move & LINK_FALL )
		{
			fallorigin = o1;
			return LINK_FALL;
		}

		o1 = v1;	//next step

		//check for reached ( which is invalid in this case )
		if( VectorDistance( o1, pn2.origin ) < 24 && AI_VisibleOrigins( o1, pn2.origin ) ) {
			return LINK_INVALID;
		}

		eternalcount++;
		if(eternalcount > 200000000 )
			gi.error ("ETERNAL COUNT\n");
	}

	return LINK_INVALID;
}


//==========================================
// AI_LadderLink_FindUpperNode
// finds upper node in same ladder, if any
//==========================================
static node_id AI_LadderLink_FindUpperNode(node_id node)
{
	float	xzdist;
	node_id	candidate = NODE_INVALID;
	nav_node &pnode = nav.nodes[node];

	for(node_id i = 0; i < nav.nodes.size(); i++)
	{
		if( i == node )
			continue;

		nav_node &pinode = nav.nodes[i];

		if (!(pinode.flags & NODEFLAGS_LADDER))
			continue;
		
		//same ladder
		vector eorg = pinode.origin - pnode.origin;
		eorg[2] = 0; //ignore height
		xzdist = VectorLength(eorg);
		if(xzdist > 8)	//not in our ladder
			continue;

		if( pnode.origin[2] > pinode.origin[2] )	//below
			continue;

		if( candidate == NODE_INVALID )
		{	//first found
			candidate = i;
			continue;
		}

		//shorter is better
		if( pinode.origin[2] - pnode.origin[2] < nav.nodes[candidate].origin[2] - pnode.origin[2] )
			candidate = i;
	}

	if( candidate != -1 )
		gi.dprintf( "LADDER: FOUND upper node in ladder\n");

	return candidate;
}

//==========================================
// AI_LadderLink_FindLowerNode
// finds lower node in same ladder, if any
//==========================================
static node_id AI_LadderLink_FindLowerNode(node_id node)
{
	vector	eorg;
	float	xzdist;
	node_id	candidate = NODE_INVALID;
	nav_node &pnode = nav.nodes[node];

	for(node_id i = 0; i < nav.nodes.size(); i++)
	{
		if( i == node )
			continue;

		nav_node &pinode = nav.nodes[i];

		if (!(pinode.flags & NODEFLAGS_LADDER))
			continue;
		
		//same ladder
		eorg = pinode.origin - pnode.origin;
		eorg[2] = 0; //ignore height
		xzdist = VectorLength(eorg);
		if(xzdist > 8)	//not in our ladder
			continue;

		if( pinode.origin[2] > pnode.origin[2] )	//above
			continue;

		if( candidate == NODE_INVALID ) {	//first found
			candidate = i;
			continue;
		}

		//shorter is better
		if( pnode.origin[2] - pinode.origin[2] < pnode.origin[2] - nav.nodes[candidate].origin[2] )
			candidate = i;
	}

	if( candidate != -1 )
		gi.dprintf( "LADDER: FOUND lower node in ladder\n");

	return candidate;
}

//==========================================
// AI_IsLadderLink
// interpretation of this link
//==========================================
static ai_link_type AI_IsLadderLink( node_id n1, node_id n2 )
{
	vector	eorg;
	float	xzdist;
	
	nav_node &pn1 = nav.nodes[n1];
	nav_node &pn2 = nav.nodes[n2];

	eorg = pn2.origin - pn1.origin;
	eorg[2] = 0; //ignore height
	
	xzdist = VectorLength(eorg);

	if(xzdist < 0) 
		xzdist = -xzdist;

	//if both are ladder nodes
	if( (pn1.flags & NODEFLAGS_LADDER) && (pn2.flags & NODEFLAGS_LADDER ))
	{
		node_id	candidate = AI_LadderLink_FindUpperNode( n1 );
		if( candidate != n2 )
			return LINK_INVALID;

		return LINK_LADDER;
	}

	//if only second is ladder node
	if( !(pn1.flags & NODEFLAGS_LADDER) && (pn2.flags & NODEFLAGS_LADDER))
	{
		if( pn1.flags & NODEFLAGS_WATER )
		{
			if( AI_VisibleOrigins(pn1.origin, pn2.origin) ) {
				if( pn2.flags & NODEFLAGS_WATER )
					return LINK_WATER;
				else
					return LINK_LADDER;
			}

			return LINK_INVALID;
		}

		//only allow linking to the bottom ladder node from outside the ladder
		node_id candidate = AI_LadderLink_FindLowerNode( n2 );
		if( candidate == -1 ) {
			if( pn2.flags & NODEFLAGS_WATER ) {
				ai_link_type link = AI_RunGravityBox( n1, n2 );
				if( link & LINK_INVALID )
					return LINK_INVALID;
				return LINK_WATER;
			} else {
				return AI_GravityBoxToLink( n1, n2 );
			}
		}
		
		return LINK_INVALID;
	}

	//if only first is ladder node
	if( (pn1.flags & NODEFLAGS_LADDER) && !(pn2.flags & NODEFLAGS_LADDER) )
	{
		//if it has a upper ladder node, it can only link to it
		node_id candidate = AI_LadderLink_FindUpperNode( n1 );
		if( candidate != NODE_INVALID )
			return LINK_INVALID;

		if( VectorDistance(pn1.origin, pn2.origin) > (NODE_DENSITY*0.8) )
			return LINK_INVALID;

		ai_link_type link = AI_RunGravityBox( n2, n1 );	//try to reach backwards
		if( link & LINK_INVALID || link & LINK_FALL )
			return LINK_INVALID;

		return LINK_LADDER;
	}

	return LINK_INVALID;
}

//==========================================
// AI_FindLinkType
// interpretation of this link
//==========================================
ai_link_type AI_FindLinkType(node_id n1, node_id n2)
{
	if( n1 == n2 || n1 == NODE_INVALID || n2 == NODE_INVALID )
		return LINK_INVALID;

	if( AI_PlinkExists( n1, n2 ))
		return LINK_INVALID; //already saved

	nav_node &pn1 = nav.nodes[n1];
	nav_node &pn2 = nav.nodes[n2];

	//ignore server links
	if( (pn1.flags & NODEFLAGS_SERVERLINK) || (pn2.flags & NODEFLAGS_SERVERLINK))
		return LINK_INVALID; // they are added only by the server at dropping entity nodes
	
	//LINK_LADDER
	if((pn1.flags & NODEFLAGS_LADDER) || (pn2.flags & NODEFLAGS_LADDER)) 
		return AI_IsLadderLink( n1, n2 );
	
	//find out the link move type against the world
	return AI_GravityBoxToLink( n1, n2 );
}


//==========================================
// AI_IsJumpLink
// interpretation of this link
//==========================================
static ai_link_type AI_IsJumpLink(node_id n1, node_id n2)
{
	ai_link_type link, climblink = LINK_NONE;

	if( n1 == n2 || n1 == NODE_INVALID || n2 == NODE_INVALID )
		return LINK_INVALID;
	
	nav_node &pn1 = nav.nodes[n1];
	nav_node &pn2 = nav.nodes[n2];

	//ignore server links
	if( (pn1.flags & NODEFLAGS_SERVERLINK) || (pn2.flags & NODEFLAGS_SERVERLINK))
		return LINK_INVALID; // they are added only by the server at dropping entity nodes
	
	//LINK_LADDER
	if( (pn1.flags & NODEFLAGS_LADDER) || (pn2.flags & NODEFLAGS_LADDER)) 
		return LINK_INVALID;

	//can't jump if begins inside water
	if( pn1.flags & NODEFLAGS_WATER )
		return LINK_INVALID;

	//find out the link move type against the world
	link = AI_RunGravityBox( n1, n2 );

	if( !(link & LINK_INVALID) )	// recheck backwards for climb links
		return LINK_INVALID;
	
	if( AI_PlinkExists( n2, n1 ))
		climblink = AI_PlinkMoveType( n2, n1 );
	else 
		climblink = AI_RunGravityBox( n2, n1 );
	
	if( climblink & LINK_FALL ) {
		climblink &= ~LINK_FALL;
		link &= ~LINK_INVALID;
		link = (link|climblink|LINK_CLIMB);
	}
	
	//see if we can jump it
	if( link & LINK_CLIMB && link & LINK_FALL )	
	{
		vector fo1, fo2;
		float	dist;
		float	heightdiff;
		
		link = AI_FindFallOrigin( n1, n2, fo1 );
		if (!(link & LINK_FALL))
			return LINK_INVALID;
		
		link = AI_FindFallOrigin( n2, n1, fo2 );
		if (!(link & LINK_FALL))
			return LINK_INVALID;
		
		//reachable? (faster)
		if( !AI_VisibleOrigins( fo1, fo2 ) )
			return LINK_INVALID;
		
		if( fo2[2] > fo1[2] + AI_JUMPABLE_HEIGHT )	//n1 is just too low
			return LINK_INVALID;
		
		heightdiff = fo2[2] - fo1[2];	//if n2 is higher, shorten xzplane dist
		if( heightdiff < 0 )
			heightdiff = 0;
		
		//xzplane dist is valid?
		fo2[2] = fo1[2];
		dist = VectorDistance( fo1, fo2 );
		if( (dist + heightdiff) < AI_JUMPABLE_DISTANCE && dist > 24 )
			return LINK_JUMP;
	}
	
	return LINK_INVALID;
}

//==========================================
// AI_LinkCloseNodes_JumpPass
// extend radius for jump over movetype links
//==========================================
uint32_t AI_LinkCloseNodes_JumpPass( node_id start )
{
	node_id		n1, n2;
	uint32_t		count = 0;
	float	pLinkRadius = NODE_DENSITY*2;
	bool	ignoreHeight = true;

	if( nav.nodes.size() < 1 )
		return 0;

	//do it for everynode in the list
	for( n1 = start; n1 < nav.nodes.size(); n1++ )
	{
		nav_node &pn1 = nav.nodes[n1];
		n2 = 0;
		n2 = AI_findNodeInRadius ( 0, pn1.origin, pLinkRadius, ignoreHeight);
		
		while (n2 != -1)
		{
			if( n1 != n2 && !AI_PlinkExists( n1, n2 ) )
			{
				ai_link_type linkType = AI_IsJumpLink( n1, n2 );
				if( linkType == LINK_JUMP )
				{
					//make sure there isn't a good 'standard' path for it
					float cost = AI_FindCost( n1, n2, (LINK_MOVE|LINK_STAIRS|LINK_FALL|LINK_WATER|LINK_WATERJUMP|LINK_CROUCH) );
					if( cost == -1 || cost > 4 ) {
						if( AI_AddLink(n1, n2, LINK_JUMP) )
							count++;
					}
				}
			}
			//next
			n2 = AI_findNodeInRadius ( n2, pn1.origin, pLinkRadius, ignoreHeight);
		}
	}

	return count;
}


//==========================================
// AI_LinkCloseNodes
// track the nodes list and find close nodes around. Link them if possible
//==========================================
uint32_t AI_LinkCloseNodes()
{
	node_id		n1, n2;
	uint32_t	count = 0;
	float	pLinkRadius = NODE_DENSITY*1.5;
	bool	ignoreHeight = true;

	//do it for everynode in the list
	for(n1=0; n1<nav.nodes.size(); n1++ )
	{
		nav_node &pn1 = nav.nodes[n1];
		n2 = AI_findNodeInRadius ( 0, pn1.origin, pLinkRadius, ignoreHeight);
		
		while (n2 != -1)
		{
			if( AI_AddLink( n1, n2, AI_FindLinkType(n1, n2) ))
				count++;
			
			n2 = AI_findNodeInRadius ( n2, pn1.origin, pLinkRadius, ignoreHeight);
		}
	}
	return count;
}

#endif