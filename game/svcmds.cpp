#include "config.h"
#include "lib/string.h"
#include "lib/gi.h"
#include "lib/types/allocator.h"
#include "svcmds.h"

void ServerCommand()
{
	string s = gi.argv(1);

	if (s == "mem")
		gi.dprintf("%u, %u\n", internal::game_count, internal::non_game_count);
}