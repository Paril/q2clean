#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "../../lib/entity.h"
#include "../game.h"
#include "ai.h"
#include "astar.h"
#include "links.h"
#include "nodes.h"
#include "aimain.h"

//==========================================
// AI_FindCost
// Determine cost of moving from one node to another
//==========================================
float AI_FindCost(node_id from, node_id to, ai_link_type movetypes)
{
	if (from == NODE_INVALID || to == NODE_INVALID)
		return -1;

	static astar_path path;

	if (!AStar_GetPath( from, to, movetypes, path))
		return -1;

	return (float)path.nodes.size();
}


//==========================================
// AI_FindClosestReachableNode
// Find the closest node to the player within a certain range
//==========================================
node_id AI_FindClosestReachableNode(vector origin, entityref passent, int32_t range, node_flags flagsmask)
{
	float		closest = FLT_MAX;
	node_id		node = NODE_INVALID;
	vector		mins, maxs;

	// For Ladders, do not worry so much about reachability
	if (flagsmask && (flagsmask & NODEFLAGS_LADDER))
		maxs = mins = vec3_origin;
	else
	{
		mins = { -15, -15, -15 };
		maxs = { 15, 15, 15 };
	}

	float rng = (float)(range * range); // square range for distance comparison (eliminate sqrt)

	for (size_t i = 0; i < nav.nodes.size(); i++)
	{
		nav_node &pnode = nav.nodes[i];

		if (!flagsmask || (pnode.flags & flagsmask))
		{
			float dist = VectorDistanceSquared(pnode.origin, origin);

			if (dist < closest && dist < rng)
			{
				// make sure it is visible
				trace tr = gi.trace(origin, mins, maxs, pnode.origin, passent, MASK_AISOLID);
				if (tr.fraction == 1.0)
				{
					node = i;
					closest = dist;
				}
			}
		}
	}

	return node;
}

//==========================================
// AI_SetupPath
//==========================================
static inline bool AI_SetupPath(entity &self, node_id from, node_id to, ai_link_type movetypes)
{
	return AStar_GetPath(from, to, movetypes, self.g.ai.path);
}

//==========================================
// AI_SetGoal
// set the goal
//==========================================
void AI_SetGoal(entity &self, node_id goal_node)
{
	self.g.ai.goal_node = goal_node;
	node_id node = AI_FindClosestReachableNode(self.s.origin, self, NODE_DENSITY * 3, NODE_ALL);

	if (node == NODE_INVALID)
	{
		AI_SetUpMoveWander(self);
		return;
	}

	//------- ASTAR -----------
	if (!AI_SetupPath(self, node, goal_node, self.g.ai.pers.moveTypesMask)) 
	{
		AI_SetUpMoveWander(self);
		return;
	}
	self.g.ai.path_position = 0;
	self.g.ai.current_node = self.g.ai.path.nodes[self.g.ai.path_position];
	//-------------------------

	if (AIDevel.debugChased && bot_showlrgoal)
		gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: GOAL: new START NODE selected %d\n", self.client->g.pers.netname.ptr(), node);

	self.g.ai.next_node = self.g.ai.current_node; // make sure we get to the nearest node first
	self.g.ai.node_timeout = 0;
}

//==========================================
// AI_FollowPath
// Move closer to goal by pointing the bot to the next node
// that is closer to the goal
//==========================================
bool AI_FollowPath(entity &self)
{
	// Show the path
	/*if (bot_showpath.intVal)
		if (AIDevel.debugChased)
			AITools_DrawPath(self, self.ai.current_node, self.ai.goal_node);*/

	if (self.g.ai.goal_node == NODE_INVALID)
		return false;

	// Try again?
	if (self.g.ai.node_timeout++ > 30)
	{
		if (self.g.ai.tries++ > 3)
			return false;
		AI_SetGoal(self, self.g.ai.goal_node);
	}

	if (self.g.ai.goal_node == NODE_INVALID)
		return false;

	// Are we there yet?
	nav_node &pcurrentNode = nav.nodes[self.g.ai.current_node];
	nav_node &pnextNode = nav.nodes[self.g.ai.next_node];
	
	float dist = VectorDistance(self.s.origin, pnextNode.origin);

	//special lower plat reached check
	if (dist < 64
		&& (pcurrentNode.flags & NODEFLAGS_PLATFORM)
		&& (pnextNode.flags & NODEFLAGS_PLATFORM)
		&& self.g.groundentity.has_value() && self.g.groundentity->g.type == ET_FUNC_PLAT)
		dist = 16;

	if ((dist < 32 && pnextNode.flags != NODEFLAGS_JUMPPAD && pnextNode.flags != NODEFLAGS_TELEPORTER_IN)
		|| (self.g.ai.status.jumpadReached && (pnextNode.flags & NODEFLAGS_JUMPPAD))
		|| (self.g.ai.status.TeleportReached && (pnextNode.flags & NODEFLAGS_TELEPORTER_IN)))
	{
		// reset timeout
		self.g.ai.node_timeout = 0;

		if (self.g.ai.next_node == self.g.ai.goal_node)
		{
			if(AIDevel.debugChased && bot_showlrgoal)
				gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: GOAL REACHED!\n", self.client->g.pers.netname.ptr());
			
			//if botroam, setup a timeout for it
			if(pnextNode.flags & NODEFLAGS_BOTROAM)
			{
				for (const auto &broam : nav.broams)
				{
					if (broam.node != self.g.ai.goal_node)
						continue;

					if(AIDevel.debugChased && bot_showlrgoal)
						gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: BotRoam Time Out set up for node %i\n", self.client->g.pers.netname.ptr(), broam.node);
					self.g.ai.status.broam_timeout_framenums[&broam - nav.broams.data()] = level.framenum + (gtime)(15.0 * BASE_FRAMERATE);
					break;
				}
			}
			
			// Pick a new goal
			AI_PickLongRangeGoal(self);
		}
		else
		{
			self.g.ai.current_node = self.g.ai.next_node;
			self.g.ai.next_node = self.g.ai.path.nodes[self.g.ai.path_position++];

			if(AIDevel.debugChased && (int32_t)bot_showpath > 1)
				gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: CurrentNode(%i):%i NextNode(%i):%i\n", self.client->g.pers.netname.ptr(), self.g.ai.current_node, nav.nodes[self.g.ai.current_node].flags, self.g.ai.next_node, nav.nodes[self.g.ai.next_node].flags);
		}
	}


	if( self.g.ai.current_node == NODE_INVALID || self.g.ai.next_node == NODE_INVALID )
		return false;

	// Set bot's movement vector
	self.g.ai.move_vector = nav.nodes[self.g.ai.next_node].origin - self.s.origin;
	return true;
}

#include <fstream>

//==========================================
// AI_LoadPLKFile
// load nodes and plinks from file
//==========================================
inline bool AI_LoadPLKFile()
{
	string filename = va("%s/%s/%s.%s", GAMEVERSION, AI_NODES_FOLDER, level.mapname.ptr(), NAV_FILE_EXTENSION);

	std::fstream stream(filename.ptr(), std::fstream::in | std::fstream::binary);

	if (!stream.is_open())
		return false;

	int32_t version;
	stream.read((char *)&version, sizeof(version));

	if (version != NAV_FILE_VERSION)
		return false;
	
	int32_t num_nodes;
	stream.read((char *)&num_nodes, sizeof(version));
	int unused;

	for (int i = 0; i < num_nodes; i++)
	{
		nav_node node;
		stream.read((char *)&node.origin, sizeof(vector));
		stream.read((char *)&node.flags, sizeof(int));
		stream.read((char *)&unused, sizeof(int));
		nav.nodes.push_back(node);
	}

	constexpr int32_t NODES_MAX_PLINKS = 16;
	
	for(int i = 0; i < num_nodes; i++)
	{
		nav_node &node = nav.nodes[i];

		int num_links;
		stream.read((char *)&num_links, sizeof(int));

		int lnodes[NODES_MAX_PLINKS];
		int ldist[NODES_MAX_PLINKS];
		int lmoveType[NODES_MAX_PLINKS];
		
		stream.read((char *)lnodes, sizeof(int) * NODES_MAX_PLINKS);
		stream.read((char *)ldist, sizeof(int) * NODES_MAX_PLINKS);
		stream.read((char *)lmoveType, sizeof(int) * NODES_MAX_PLINKS);
		
		for (int j = 0; j < num_links; j++)
			node.links.push_back({ (node_id) lnodes[j], (uint32_t) ldist[j], (ai_link_type) lmoveType[j] });
	}

	return true;
}

//==========================================
// AI_IsPlatformLink
// interpretation of this link type
//==========================================
inline ai_link_type AI_IsPlatformLink( node_id n1, node_id n2 )
{
	nav_node &pn1 = nav.nodes[n1];
	nav_node &pn2 = nav.nodes[n2];
	
	if( (pn1.flags & NODEFLAGS_PLATFORM) && (pn2.flags & NODEFLAGS_PLATFORM))
		//the link was added by it's dropping function or it's invalid
		return LINK_INVALID;

	//if first is plat but not second
	if( (pn1.flags & NODEFLAGS_PLATFORM) && !(pn2.flags & NODEFLAGS_PLATFORM) )
	{
		entityref	n1ent;
		node_id		othernode = NODE_INVALID;

		// find ent
		for(const auto &ent : nav.ents)
			if(ent.node == n1 )
				n1ent = ent.ent;

		// find the other node from that ent
		for(const auto &ent : nav.ents)
			if( ent.node != n1 && ent.ent == n1ent)
				othernode = ent.node;

		if( othernode == NODE_INVALID || !n1ent.has_value() )
			return LINK_INVALID;

		nav_node &pothernode = nav.nodes[othernode];

		//find out if n1 is the upper or the lower plat node
		if( pn1.origin[2] < pothernode.origin[2] )
			//n1 is plat lower: it can't link TO anything but upper plat node
			return LINK_INVALID;
			
		float	heightdiff;
		//n1 is plat upper: it can link to visibles at same height
		trace tr = gi.traceline(pn1.origin, pn2.origin, n1ent, MASK_NODESOLID );
		if (tr.fraction == 1.0 && !tr.startsolid) 
		{
			heightdiff = fabs(pn1.origin[2] - pn2.origin[2]);

			if( heightdiff < AI_JUMPABLE_HEIGHT )
				return LINK_MOVE;

			return LINK_INVALID;
		}
	}

	//only second is plat node
	if( !(pn1.flags & NODEFLAGS_PLATFORM) && (pn2.flags & NODEFLAGS_PLATFORM ))
	{
		entityref	n2ent;
		node_id		othernode = NODE_INVALID;

		// find ent
		for(const auto &ent : nav.ents)
			if( ent.node == n2 )
				n2ent = ent.ent;

		// find the other node from that ent
		for(const auto &ent : nav.ents)
			if( ent.node != n2 && ent.ent == n2ent)
				othernode = ent.node;

		if( othernode == NODE_INVALID || !n2ent.has_value() )
			return LINK_INVALID;

		nav_node &pothernode = nav.nodes[othernode];

		//find out if n2 is the upper or the lower plat node
		if( pn2.origin[2] < pothernode.origin[2] )
		{
			float	heightdiff;

			//n2 is plat lower: other's can link to it when visible and good height
			trace tr = gi.traceline(pn1.origin, pn2.origin, n2ent, MASK_NODESOLID );
			if (tr.fraction == 1.0 && !tr.startsolid) 
			{
				heightdiff = fabs(pn1.origin[2] - pn2.origin[2]);

				if( heightdiff < AI_JUMPABLE_HEIGHT )
					return LINK_MOVE;

				return LINK_INVALID;
			}

		}
		else
			//n2 is plat upper: others can't link to plat upper nodes
			return LINK_INVALID;
	}

	return LINK_INVALID;
}

//==========================================
// AI_IsJumpPadLink
// interpretation of this link type
//==========================================
inline ai_link_type AI_IsJumpPadLink( node_id n1, node_id n2 )
{
	nav_node &pn1 = nav.nodes[n1];
	if( pn1.flags & NODEFLAGS_JUMPPAD )
		return LINK_INVALID; //only can link TO jumppad land, and it's linked elsewhere

	nav_node &pn2 = nav.nodes[n2];
	if( pn2.flags & NODEFLAGS_JUMPPAD_LAND )
		return LINK_INVALID; //linked as TO only from it's jumppad. Handled elsewhere

	return AI_GravityBoxToLink( n1, n2 );
}

//==========================================
// AI_IsTeleporterLink
// interpretation of this link type
//==========================================
inline ai_link_type AI_IsTeleporterLink( node_id n1, node_id n2 )
{
	nav_node &pn1 = nav.nodes[n1];
	if( pn1.flags & NODEFLAGS_TELEPORTER_IN )
		return LINK_INVALID;

	nav_node &pn2 = nav.nodes[n2];
	if (pn2.flags & NODEFLAGS_TELEPORTER_OUT)
		return LINK_INVALID;

	//find out the link move type against the world
	return AI_GravityBoxToLink( n1, n2 );
}

//==========================================
// AI_FindServerLinkType
// determine what type of link it is
//==========================================
inline ai_link_type AI_FindServerLinkType( node_id n1, node_id n2 )
{
	if( AI_PlinkExists( n1, n2 ))
		return LINK_INVALID;	//already saved

	nav_node &pn1 = nav.nodes[n1];
	nav_node &pn2 = nav.nodes[n2];
	
	if( (pn1.flags & NODEFLAGS_PLATFORM) || (pn2.flags & NODEFLAGS_PLATFORM))
		return AI_IsPlatformLink(n1, n2);
	else if( (pn2.flags & NODEFLAGS_TELEPORTER_IN) || (pn1.flags & NODEFLAGS_TELEPORTER_OUT ))
		return AI_IsTeleporterLink(n1, n2);
	else if( (pn2.flags & NODEFLAGS_JUMPPAD) || (pn1.flags & NODEFLAGS_JUMPPAD_LAND ))
		return AI_IsJumpPadLink(n1, n2);

	return LINK_INVALID;
}

//==========================================
// AI_LinkServerNodes
// link the new nodes to&from those loaded from disk
//==========================================
inline uint32_t AI_LinkServerNodes( node_id start )
{
	node_id		n2;
	uint32_t	count = 0;
	float	pLinkRadius = NODE_DENSITY*1.2f;
	bool	ignoreHeight = true;

	if( start >= nav.nodes.size() )
		return 0;

	for(node_id n1 = start; n1 < nav.nodes.size(); n1++)
	{
		nav_node &pn1 = nav.nodes[n1];
		n2 = AI_findNodeInRadius ( 0, pn1.origin, pLinkRadius, ignoreHeight);
		
		while (n2 != NODE_INVALID)
		{	
			nav_node &pn2 = nav.nodes[n2];
			if( (pn1.flags & NODEFLAGS_SERVERLINK) || (pn2.flags & NODEFLAGS_SERVERLINK))
			{
				if( AI_AddLink( n1, n2, AI_FindServerLinkType(n1, n2) ) )
					count++;
				
				if( AI_AddLink( n2, n1, AI_FindServerLinkType(n2, n1) ) )
					count++;
			}
			else
			{
				if( AI_AddLink( n1, n2, AI_FindLinkType(n1, n2) ) )
					count++;
				
				if( AI_AddLink( n2, n1, AI_FindLinkType(n2, n1) ) )
					count++;
			}
			
			n2 = AI_findNodeInRadius ( n2, pn1.origin, pLinkRadius, ignoreHeight);
		}
	}
	return count;
}


//==========================================
// AI_InitNavigationData
// Setup nodes & links for this map
//==========================================
void AI_InitNavigationData()
{
	//Init nodes arrays
	nav.nodes.clear();

	//Load nodes from file
	nav.loaded = AI_LoadPLKFile();
	if( !nav.loaded )
	{
		gi.dprintf( "AI: FAILED to load nodes file.\n");
		return;
	}
	
	uint32_t linkscount = 0;

	for (const auto &node : nav.nodes)
		linkscount += node.links.size();

	uint32_t servernodesstart = nav.nodes.size();

	//create nodes for map entities
	AI_CreateNodesForEntities();
	uint32_t newlinks = AI_LinkServerNodes( servernodesstart );
	uint32_t newjumplinks = AI_LinkCloseNodes_JumpPass( servernodesstart );
	
	gi.dprintf("-------------------------------------\n" );
	gi.dprintf("       : AI: Nodes Initialized.\n" );
	gi.dprintf("       : loaded nodes:%i.\n", servernodesstart );
	gi.dprintf("       : added nodes:%i.\n", nav.nodes.size() - servernodesstart );
	gi.dprintf("       : total nodes:%i.\n", nav.nodes.size() );
	gi.dprintf("       : loaded links:%i.\n", linkscount );
	gi.dprintf("       : added links:%i.\n", newlinks );
	gi.dprintf("       : added jump links:%i.\n", newjumplinks );
}


#endif