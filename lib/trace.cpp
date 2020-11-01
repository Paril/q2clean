#include "trace.h"
#include "surface.h"
#include "entity.h"

trace::trace() :
	surface(null_surface),
	contents(CONTENTS_NONE),
	ent(world)
{
}