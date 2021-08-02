#include "config.h"
#include "entity_types.h"

entity_type ET_UNKNOWN("unknown");

/*static*/ void entity_type::register_et(entity_type *type)
{
	type->next = ET_UNKNOWN.next;
	ET_UNKNOWN.next = type;
}