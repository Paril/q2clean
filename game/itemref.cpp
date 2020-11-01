#include "items.h"
#include "itemref.h"
#include "itemlist.h"

// initialize a reference to a specific item
itemref::itemref(const gitem_id &ref) :
	_ptr(&GetItemByIndex(ref))
{
}

// check whether this itemref is holding an item
// reference or not
bool itemref::has_value() const
{
	return _ptr->id;
}