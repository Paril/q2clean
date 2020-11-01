#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "../../lib/entity.h"
#include "../player.h"
#include "../game.h"
#include "aimain.h"

///////////////////////////////////////////////////////////////////////
// Find a free client spot
///////////////////////////////////////////////////////////////////////
static inline entityref BOT_FindFreeClient()
{
	entityref bot;
	int32_t max_count = 0;
	
	// This is for the naming of the bots
	for (int32_t i = game.maxclients; i > 0; i--)
	{
		bot = itoe(i);
		
		if (bot->g.count > max_count)
			max_count = bot->g.count;
	}

	// Check for free spot
	for (int32_t i = game.maxclients; i > 0; i--)
	{
		bot = itoe(i);

		if (!bot->inuse)
			break;
	}

	if (bot->inuse)
		return nullptr;

	bot->g.count = max_count + 1; // Will become bot name...
	return bot;
}

static stringlit bot_skins[] = {
	"female/athena",
	"male/grunt"
};

///////////////////////////////////////////////////////////////////////
// Set the name of the bot and update the userinfo
///////////////////////////////////////////////////////////////////////
static string BOT_SetName(entity &bot, stringref name, stringref skin, stringref team)
{
	// name
	stringref bot_name = name ? name : (stringref)va("Bot%d", bot.g.count);

	// skin
	string bot_skin = skin ? skin : bot_skins[Q_rand_uniform(lengthof(bot_skins))];

	// initialise userinfo
	return strconcat("\\name\\", bot_name, "\\skin\\", bot_skin, "\\hand\\2");
}

///////////////////////////////////////////////////////////////////////
// Spawn the bot
///////////////////////////////////////////////////////////////////////
void BOT_SpawnBot(stringref team, stringref name, stringref skin, string userinfo)
{
	if(!nav.loaded )
	{
		gi.dprintf("Can't spawn bots without a valid navigation file\n");
		return;
	}
	
	entityref bot = BOT_FindFreeClient();
	
	if (!bot.has_value())
	{
		gi.dprintf("Server is full, increase Maxclients.\n");
		return;
	}

	//init the bot
	bot->g.ai.is_bot = true;
	bot->g.yaw_speed = 100;

	// To allow bots to respawn
	if (!userinfo)
		userinfo = BOT_SetName(bot, name, skin, team);

	ClientConnect (bot, userinfo);
	
	ClientBegin(bot);

	bot->g.ai.pers.skillLevel = Q_rand_uniform(MAX_BOT_SKILL);

	bot->g.think = AI_Think;
	bot->g.nextthink = level.framenum + 1;
}

///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void BOT_RemoveBot(stringref name)
{
	bool freed=false;

	for (entity &bot : entity_range(1, game.maxclients))
	{
		if(bot.inuse && bot.g.ai.is_bot && (name == "all" || bot.client->g.pers.netname == name))
		{
			ClientDisconnect(bot);
			freed = true;
		}
	}

	if(!freed)	
		gi.dprintf("%s not found\n", name.ptr());
}

#endif