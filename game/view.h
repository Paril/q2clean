#pragma once

import "entity_types.h";
import "game.h";

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
