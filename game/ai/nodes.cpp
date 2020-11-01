#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "../../lib/entity.h"
#include "../util.h"
#include "ai.h"
#include "links.h"
#include "navigation.h"

//==========================================
// AI_DropNodeOriginToFloor
//==========================================
inline bool AI_DropNodeOriginToFloor(vector &origin, entityref passent)
{
	trace tr = gi.trace(origin, { -15, -15, 0 }, { 15, 15, 0 }, origin - vector { 0, 0, 2048 }, passent, MASK_NODESOLID);

	if (tr.startsolid)
		return false;

	origin = tr.endpos + vector { 0, 0, 24 };
	return true;
}

//==========================================
// AI_FlagsForNode
// check the world and set up node flags
//==========================================
inline node_flags AI_FlagsForNode(vector origin, entityref passent)
{
	node_flags flagsmask = NODEFLAGS_NONE;

	//water
	if (gi.pointcontents(origin) & MASK_WATER)
		flagsmask |= NODEFLAGS_WATER;

	//floor
	trace tr = gi.trace(origin, { -15, -15, 0 }, { 15, 15, 0 }, origin - vector { 0, 0, AI_JUMPABLE_HEIGHT }, passent, MASK_NODESOLID);

	if (tr.fraction >= 1.0 )
		flagsmask |= NODEFLAGS_FLOAT;

	return flagsmask;
}


//==========================================
// AI_PredictJumpadDestity
// Make a guess on where a jumpad will send
// the player.
//==========================================
inline bool AI_PredictJumpadDestity(entity &ent, vector &outvec)
{
	trace	tr;
	vector	pad_origin, v1, v2;
	float	htime, vtime, tmpfloat, player_factor;	//player movement guess
	vector	floor_target_origin, target_origin;
	vector	floor_dist_vec, floor_movedir;

	// get target entity
	entityref etarget = G_FindFunc(nullptr, g.targetname, ent.g.target, striequals);
	if (!etarget.has_value())
		return false;

	// find pad origin
	v1 = ent.maxs;
	v2 = ent.mins;
	pad_origin[0] = (v1[0] - v2[0]) / 2 + v2[0];
	pad_origin[1] = (v1[1] - v2[1]) / 2 + v2[1];
	pad_origin[2] = ent.maxs[2];

	//make a projection 'on floor' of target origin
	target_origin = etarget->s.origin;
	floor_target_origin = etarget->s.origin;
	floor_target_origin[2] = pad_origin[2];	//put at pad's height

	//make a guess on how player movement will affect the trajectory
	tmpfloat = VectorDistance( pad_origin, floor_target_origin );
	htime = sqrt(tmpfloat);
	vtime = sqrt(etarget->s.origin[2] - pad_origin[2]);
	if (!vtime)
		return false;
	htime *= 4;
	vtime *= 4;
	if(htime > vtime)
		htime = vtime;
	player_factor = vtime - htime;

	// find distance vector, on floor, from pad_origin to target origin.
	floor_dist_vec = floor_target_origin - pad_origin;

	// movement direction on floor
	floor_movedir = floor_dist_vec;
	VectorNormalize(floor_movedir);

	// move both target origin and target origin on floor by player movement factor.
	target_origin += floor_movedir * player_factor;
	floor_target_origin += floor_movedir * player_factor;

	// move target origin on floor by floor distance, and add another player factor step to it
	floor_target_origin += floor_dist_vec;
	floor_target_origin += floor_movedir * player_factor;

	//trace from target origin to endPoint.
	tr = gi.trace(target_origin, { -15, -15, -8 }, { 15, 15, 8 }, floor_target_origin, 0, MASK_NODESOLID);
	if ((tr.fraction == 1.0 && tr.startsolid) || (tr.allsolid && tr.startsolid))
	{
		gi.dprintf("JUMPAD LAND: ERROR: trace was in solid.\n"); //started inside solid (target should never be inside solid, this is a mapper error)
		return false;
	}
	else if ( tr.fraction == 1.0 )
	{
		//didn't find solid. Extend Down (I have to improve this part)
		vector	target_origin2, extended_endpoint, extend_dist_vec;
		
		target_origin2 = floor_target_origin;
		extend_dist_vec = floor_target_origin - target_origin;
		
		extended_endpoint = target_origin2 + extend_dist_vec;
		//repeat tracing
		tr = gi.trace(target_origin2, { -15, -15, -8 }, { 15, 15, 8 }, extended_endpoint, 0, MASK_NODESOLID);
		if ( tr.fraction == 1.0 )
			return false;//still didn't find solid
	}

	outvec = tr.endpos;
	return true;
}

//==========================================
// AI_AddNode_JumpPad
// Drop two nodes, one at jump pad and other
// at predicted destity
//==========================================
inline node_id AI_AddNode_JumpPad(entity &ent)
{
	vector v1,v2;
	vector out;

	if( !AI_PredictJumpadDestity( ent, out )) 
		return NODE_INVALID;
	
	// jumpad node
	nav_node node;
	node.flags = (NODEFLAGS_JUMPPAD|NODEFLAGS_SERVERLINK|NODEFLAGS_REACHATTOUCH);

	// find the origin
	v1 = ent.maxs;
	v2 = ent.mins;
	
	node.origin[0] = (v1[0] - v2[0]) / 2 + v2[0];
	node.origin[1] = (v1[1] - v2[1]) / 2 + v2[1];
	node.origin[2] = ent.maxs[2] + 16;	//raise it up a bit

	node.flags |= AI_FlagsForNode( node.origin, nullptr);
	
	nav.nodes.push_back(node);
	
	// Destiny node
	node.flags = (NODEFLAGS_JUMPPAD_LAND|NODEFLAGS_SERVERLINK);
	node.origin = out;
	AI_DropNodeOriginToFloor(node.origin, nullptr);

	node.flags |= AI_FlagsForNode( node.origin, nullptr);
	
	node_id newest = nav.nodes.size();
	nav.nodes.push_back(node);
	
	// link jumpad to dest
	AI_AddLink(newest - 1, newest, LINK_JUMPPAD );
	return newest - 1;
}

//==========================================
// AI_AddNode_Door
// Drop a node at each side of the door
// and force them to link. Only typical
// doors are covered.
//==========================================
static node_id AI_AddNode_Door( entity &ent )
{
	entityref	other;
	vector	mins, maxs;
	vector	forward, right, up;
	vector	door_origin;

	if (ent.g.flags & FL_TEAMSLAVE)
		return NODE_INVALID;		//only team master will drop the nodes

	//make box formed by all team members boxes
	mins = ent.absmin;
	maxs = ent.absmax;

	for (other = ent.g.teamchain ; other.has_value(); other = other->g.teamchain)
	{
		AddPointToBounds (other->absmin, mins, maxs);
		AddPointToBounds (other->absmax, mins, maxs);
	}

	door_origin = (maxs - mins) / 2 + mins;

	//now find the crossing angle
	AngleVectors( ent.s.angles, &forward, &right, &up );
	VectorNormalize( right );

	//add node
	nav_node node;
	node.flags = NODEFLAGS_NONE/*NODEFLAGS_SERVERLINK*/;
	node.origin = door_origin + right * 32;
	AI_DropNodeOriginToFloor( node.origin, 0);
	node.flags |= AI_FlagsForNode( node.origin, 0 );
	nav.nodes.push_back(node);

	//add node 2
	node.flags = NODEFLAGS_NONE/*NODEFLAGS_SERVERLINK*/;
	node.origin = door_origin + -32 * right;
	AI_DropNodeOriginToFloor( node.origin, 0 );
	node.flags |= AI_FlagsForNode( node.origin, 0 );
	
	uint32_t newest = nav.nodes.size();
	nav.nodes.push_back(node);
	
	//add links in both directions
	AI_AddLink( newest, newest-1, LINK_MOVE );
	AI_AddLink( newest-1, newest, LINK_MOVE );

	return newest-1;
}

//==========================================
// AI_AddNode_Platform
// drop two nodes one at top, one at bottom
//==========================================
static node_id AI_AddNode_Platform( entity &ent )
{
	vector v1,v2;

	// Upper node
	nav_node node;
	node.flags = (NODEFLAGS_PLATFORM|NODEFLAGS_SERVERLINK|NODEFLAGS_FLOAT);
	v1 = ent.maxs;
	v2 = ent.mins;
	node.origin[0] = (v1[0] - v2[0]) / 2 + v2[0];
	node.origin[1] = (v1[1] - v2[1]) / 2 + v2[1];
	node.origin[2] = ent.maxs[2] + 8;

	node.flags |= AI_FlagsForNode( node.origin, 0 );

	//put into ents table
	nav.ents.push_back({ ent, nav.nodes.size() });
	
	nav.nodes.push_back(node);
	
	// Lower node
	vector priorNodeOrigin = node.origin;
	
	node.flags = (NODEFLAGS_PLATFORM|NODEFLAGS_SERVERLINK|NODEFLAGS_FLOAT);
	node.origin[0] = priorNodeOrigin[0];
	node.origin[1] = priorNodeOrigin[1];
	node.origin[2] = ent.mins[2] + (AI_JUMPABLE_HEIGHT - 1);

	node.flags |= AI_FlagsForNode( node.origin, 0 );

	//put into ents table
	nav.ents.push_back({ ent, nav.nodes.size() });

	uint32_t newest = nav.nodes.size();
	
	nav.nodes.push_back(node);

	// link lower to upper
	AI_AddLink( newest, newest-1, LINK_PLATFORM );

	//next
	return newest-1;
}

//==========================================
// AI_AddNode_Teleporter
// Drop two nodes, one at trigger and other
// at target entity
//==========================================
static node_id AI_AddNode_Teleporter( entity &ent )
{
	entityref dest = G_FindFunc(nullptr, g.targetname, ent.g.target, striequals);
	if (!dest.has_value())
		return NODE_INVALID;
	
	//NODE_TELEPORTER_IN
	nav_node node;
	node.flags = (NODEFLAGS_TELEPORTER_IN|NODEFLAGS_SERVERLINK);
	
	vector v1 = ent.maxs;
	vector v2 = ent.mins;
	node.origin[0] = (v1[0] - v2[0]) / 2 + v2[0];
	node.origin[1] = (v1[1] - v2[1]) / 2 + v2[1];
	node.origin[2] = ent.mins[2]+32;

	node.flags |= AI_FlagsForNode( node.origin, ent );
	
	nav.nodes.push_back(node);
	
	//NODE_TELEPORTER_OUT
	node.flags = (NODEFLAGS_TELEPORTER_OUT|NODEFLAGS_SERVERLINK);
	node.origin = dest->s.origin;
	if ( ent.g.spawnflags & 1 ) // droptofloor
		node.flags |= NODEFLAGS_FLOAT;
	else
		AI_DropNodeOriginToFloor( node.origin, 0 );

	node.flags |= AI_FlagsForNode( node.origin, ent );
	
	// link from teleport_in
	uint32_t newest = nav.nodes.size();
	
	nav.nodes.push_back(node);
	
	AI_AddLink( newest - 1, newest, LINK_TELEPORT );

	return newest - 1;
}

//==========================================
// AI_AddNode_BotRoam
// add nodes from bot roam entities
//==========================================
static node_id AI_AddNode_BotRoam( entity &ent )
{
	nav_node node;
	node.flags = (NODEFLAGS_BOTROAM);//bot roams are not NODEFLAGS_NOWORLD

	// Set location
	node.origin = ent.s.origin;
	if ( ent.g.spawnflags & 1 ) // floating items
		node.flags |= NODEFLAGS_FLOAT;
	else if( !AI_DropNodeOriginToFloor(node.origin, 0) )
		return NODE_INVALID;//spawned inside solid

	node.flags |= AI_FlagsForNode( node.origin, 0);
	
	uint32_t newest = nav.nodes.size();
	
	nav.nodes.push_back(node);

	//count into bot_roams table
	float weight;
	if( ent.g.count )
		weight = ent.g.count * 0.01f;//count is a int with a value in between 0 and 100
	else
		weight = 0.3f;
	
	nav.broams.push_back({
		newest,
		weight
	});

	return newest-1; // return the node added
}

//==========================================
// AI_AddNode_ItemNode
// Used to add nodes from items
//==========================================
static node_id AI_AddNode_ItemNode( entity &ent )
{
	nav_node node;
	
	node.origin = ent.s.origin;
	if ( ent.g.spawnflags & 1 ) // floating items
		node.flags |= NODEFLAGS_FLOAT;
	else if( !AI_DropNodeOriginToFloor( node.origin, 0 ) )
		return NODE_INVALID;	//spawned inside solid

	node.flags |= AI_FlagsForNode( node.origin, 0 );

	nav.nodes.push_back(node);

	return nav.nodes.size() - 1; // return the node added
}

inline node_id NodeIDFromNode(const nav_node &node)
{
	return &node - nav.nodes.data();
}

//==========================================
// AI_CreateNodesForEntities
//
// Entities aren't saved into nodes files anymore.
// They generate and link it's nodes at map load.
//==========================================
void AI_CreateNodesForEntities()
{
	nav.ents.clear();

	// Add special entities
	for(auto &ent : entity_range(0, num_entities))
	{
		if( !ent.g.type )
			continue;

		// platforms
		if(ent.g.type == ET_FUNC_PLAT)
			AI_AddNode_Platform( ent );
		// teleporters
		//else if(ent.g.type == ET_TRIGGER_TELEPORT)
		//	AI_AddNode_Teleporter( ent );
		// jump pads
		else if(ent.g.type == ET_TRIGGER_PUSH)
			AI_AddNode_JumpPad( ent );
		// doors
		else if(ent.g.type == ET_FUNC_DOOR)
			AI_AddNode_Door( ent );
	}

	// bot roams
	nav.broams.clear();

	//visit world nodes first, and put in list what we find in there
	for (const auto &node : nav.nodes)
		if (node.flags & NODEFLAGS_BOTROAM)
			nav.broams.push_back({ NodeIDFromNode(node), 0.3f });

	//now add bot roams from entities
	for (auto &ent : entity_range(0, num_entities))
	{
		if( !ent.g.type )		
			continue;

		/*if(ent.g.type == ET_BOTROAM)
		{
			//if we have a available node close enough to the item, use it instead of dropping a new node
			node_id node = AI_FindClosestReachableNode( ent.s.origin, 0, 48, NODE_ALL );
			if( node != -1)
			{
				nav_node &pnode = nav.nodes[node];

				if (!(pnode.flags & (NODEFLAGS_SERVERLINK | NODEFLAGS_LADDER)))
				{
					float heightdiff = fabs(ent.s.origin[2] - pnode.origin[2]);
					
					if( heightdiff < AI_STEPSIZE) //near enough
					{
						pnode.flags |= NODEFLAGS_BOTROAM;
						
						float weight;
						
						//add node to botroam list
						if( ent.g.count )
							weight = ent.g.count * 0.01f;//count is a int with a value in between 0 and 100
						else
							weight = 0.3f; //jalfixme: add cmd to weight (dropped by console cmd, self is player)
						
						nav().broams.push_back({ node, weight });
						continue;
					}
				}
			}

			//drop a new node
			AI_AddNode_BotRoam( ent );
		}*/
	}

	// game items (I want all them on top of the nodes array)
	nav.items.clear();

	for(auto &ent : entity_range(0, num_entities))
	{
		if( ent.g.type != ET_ITEM )		
			continue;

		//if we have a available node close enough to the item, use it
		node_id node = AI_FindClosestReachableNode( ent.s.origin, 0, 48, NODE_ALL );

		if( node != NODE_INVALID )
		{
			nav_node &pnode = nav.nodes[node];

			if (pnode.flags & (NODEFLAGS_SERVERLINK | NODEFLAGS_LADDER))
				node = NODE_INVALID;
			else
			{
				float heightdiff = fabs(ent.s.origin[2] - pnode.origin[2]);
				
				if( heightdiff > AI_STEPSIZE )	//not near enough
					node = NODE_INVALID;
			}
		}
		
		//drop a new node
		if( node == NODE_INVALID )
			node = AI_AddNode_ItemNode( ent );

		if( node != NODE_INVALID )
			nav.items.push_back({ ent.g.item->id, ent, node });
	}
}

#endif