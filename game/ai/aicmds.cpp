#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/gi.h"
#include "aispawn.h"

//==========================================
// BOT_ServerCommand
// Special server command processor
//==========================================
bool BOT_ServerCommand()
{
	string cmd = strlwr(gi.argv(1));

	if (cmd == "addbot")
		BOT_SpawnBot (gi.argv(2), gi.argv(3), gi.argv(4), nullptr);
    else if(cmd == "removebot")
    	BOT_RemoveBot(gi.argv(2));
   /*
	else if( !Q_stricmp (cmd, "editnodes") )
		AITools_InitEditnodes();

	else if( !Q_stricmp (cmd, "makenodes") )
		AITools_InitMakenodes();

	else if( !Q_stricmp (cmd, "savenodes") )
		AITools_SaveNodes();

	else if( !Q_stricmp (cmd, "addbotroam") )
		AITools_AddBotRoamNode();
	*/
	else
		return false;

	return true;
}

#endif