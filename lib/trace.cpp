#include "trace.h"
#include "surface.h"
#include "server_entity.h"

trace::trace() :
	surface(null_surface),
	contents(CONTENTS_NONE),
	ent(world)
{
}