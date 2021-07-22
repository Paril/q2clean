#pragma once

#include "entity_types.h"
#include "game.h"

// view pitching times
constexpr float DAMAGE_TIME = 0.5f;

/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
void ClientEndServerFrame(entity &ent);
