#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "astar.h"
#include "ai.h"

enum astar_node_list : uint8_t
{
	NOLIST,
	OPENLIST,
	CLOSEDLIST
};

struct astarnode
{
	node_id		parent;
	int32_t		G;
	int32_t		H;

	astar_node_list	list;
};

//list contains all studied nodes, Open and Closed together
static dynarray<node_id> alist;

static dynarray<astarnode> astarnodes;

static dynarray<node_id> Apath;

static node_id originNode;
static node_id goalNode;
static node_id currentNode;

ai_link_type ValidLinksMask;

static inline bool AStar_nodeIsInPath(node_id node)
{
	for (const auto &p : Apath) 
		if (node == p)
			return true;

	return false;
}

static inline bool AStar_nodeIsInClosed(node_id node)
{
	return astarnodes[node].list == CLOSEDLIST;
}

static inline bool AStar_nodeIsInOpen(node_id node)
{
	return astarnodes[node].list == OPENLIST;
}

static inline void AStar_InitLists()
{
	Apath.clear();
	alist.clear();
	astarnodes.clear();
	astarnodes.resize(nav.nodes.size());
}

static uint32_t AStar_PLinkDistance(node_id n1, node_id n2)
{
	for (const auto &link : nav.nodes[n1].links)
		if (link.node == n2)
			return link.dist;

	return (uint32_t)-1;
}

static inline uint32_t Astar_HDist_ManhatanGuess(node_id node)
{
	//teleporters are exceptional
	if (nav.nodes[node].flags & NODEFLAGS_TELEPORTER_IN)
		node++; //it's tele out is stored in the next node in the array

	//use only positive values. We don't care about direction.
	vector DistVec = (nav.nodes[goalNode].origin - nav.nodes[node].origin).each(fabs);

	return (uint32_t)(DistVec[0] + DistVec[1] + DistVec[2]);
}

static inline void AStar_PutInClosed(node_id node)
{
	if (!astarnodes[node].list)
		alist.push_back(node);

	astarnodes[node].list = CLOSEDLIST;
}

static inline void AStar_PutAdjacentsInOpen(node_id node)
{
	for (const auto &link : nav.nodes[node].links)
	{
		//ignore invalid links
		if (!(ValidLinksMask & link.moveType))
			continue;

		node_id addnode = link.node;

		//ignore self
		if (addnode == node)
			continue;

		//ignore if it's already in closed list
		if (AStar_nodeIsInClosed(addnode))
			continue;

		astarnode &paddnode = astarnodes[addnode];
		const astarnode &pstarnode = astarnodes[node];

		//if it's already inside open list
		if (AStar_nodeIsInOpen(addnode))
		{
			int plinkDist = AStar_PLinkDistance(node, addnode);

			if (plinkDist == -1)
				gi.dprintf("WARNING: AStar_PutAdjacentsInOpen - Couldn't find distance between nodes\n");
			//compare G distances and choose best parent
			else if (paddnode.G > (pstarnode.G + plinkDist))
			{
				paddnode.parent = node;
				paddnode.G = pstarnode.G + plinkDist;
			}
		}
		else
		{	//just put it in
			int plinkDist = AStar_PLinkDistance(node, addnode);
			if (plinkDist == -1)
			{
				plinkDist = AStar_PLinkDistance(addnode, node);
				if (plinkDist == -1)
					plinkDist = 999;//jalFIXME

				//ERROR
				gi.dprintf("WARNING: AStar_PutAdjacentsInOpen - Couldn't find distance between nodes\n");
			}

			//put in global list
			if (!paddnode.list)
				alist.push_back(addnode);

			paddnode.parent = node;
			paddnode.G = pstarnode.G + plinkDist;
			paddnode.H = Astar_HDist_ManhatanGuess(addnode);
			paddnode.list = OPENLIST;
		}
	}
}

static inline node_id AStar_FindInOpen_BestF()
{
	int32_t	bestF = NODE_INVALID;
	node_id best = NODE_INVALID;

	for (const auto &node : alist)
	{
		astarnode &pnode = astarnodes[node];

		if (pnode.list != OPENLIST)
			continue;

		if (bestF == NODE_INVALID || bestF > (pnode.G + pnode.H))
		{
			bestF = pnode.G + pnode.H;
			best = node;
		}
	}

	return best;
}

static inline void AStar_ListsToPath()
{
	for (node_id cur = goalNode; cur != originNode; cur = astarnodes[cur].parent) 
		Apath.insert(Apath.begin(), cur);
}

static inline bool AStar_FillLists()
{
	//put current node inside closed list
	AStar_PutInClosed(currentNode);
	
	//put adjacent nodes inside open list
	AStar_PutAdjacentsInOpen(currentNode);
	
	//find best adjacent and make it our current
	currentNode = AStar_FindInOpen_BestF();

	return currentNode != NODE_INVALID;	//if -1 path is bloqued
}

bool AStar_ResolvePath(node_id n1, node_id n2, ai_link_type movetypes)
{
	ValidLinksMask = movetypes;
	if (!ValidLinksMask)
		ValidLinksMask = DEFAULT_MOVETYPES_MASK;

	AStar_InitLists();

	originNode = n1;
	goalNode = n2;
	currentNode = originNode;

	while (!AStar_nodeIsInOpen(goalNode))
		if (!AStar_FillLists())
			return false;	//failed

	AStar_ListsToPath();
	return true;
}

bool AStar_GetPath(node_id origin, node_id goal, ai_link_type movetypes, astar_path &path)
{
	if (!AStar_ResolvePath(origin, goal, movetypes))
		return false;

	path.originNode = origin;
	path.goalNode = goal;
	path.nodes = std::move(Apath);
	return true;
}

#endif