#include "../lib/types.h"
#ifdef BOTS
#include "ai/aicmds.h"
#endif

void ServerCommand()
{
#ifdef BOTS
	if (BOT_ServerCommand ())
		return;
#endif
}