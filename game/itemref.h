#pragma once

#include "../lib/types.h"

struct gitem_t;

// thrown on an invalid item dereference
class invalid_itemref : public std::exception
{
public:
	invalid_itemref() :
		std::exception("attempted to dereference a null item", 1)
	{
	}
};

// itemref holds a reference to a valid item, or nullptr.
struct itemref
{
private:
	const gitem_t	*_ptr;

public:
	// initialize an empty reference
	inline itemref() :
		itemref(ITEM_NONE)
	{
	}

	// initialize an empty reference
	inline itemref(std::nullptr_t) :
		itemref()
	{
	}

	// initialize a reference to a specific item
	inline itemref(const gitem_t &ref) :
		_ptr(&ref)
	{
	}

	// initialize a reference to a specific item
	itemref(const gitem_id &ref);

	// check whether this itemref is holding an item
	// reference or not
	bool has_value() const;

	// check if this itemref holds the same reference as another itemref
	inline bool operator==(const itemref &rhs) const { return _ptr == rhs._ptr; }

	// check if this itemref doesn't hold the same reference as another itemref
	inline bool operator!=(const itemref &rhs) const { return _ptr != rhs._ptr; }

	// check if this itemref holds this item
	inline bool operator==(const gitem_t &rhs) const { return _ptr == &rhs; }

	// check if this itemref doesn't hold this item
	inline bool operator!=(const gitem_t &rhs) const { return _ptr != &rhs; }

	// you can use -> to easily dereference an item reference
	// for quick access checks before copying it out. this won't
	// throw if the item is invalid, but just returns the null item's
	// info.
	inline const gitem_t *operator->()
	{
		return _ptr;
	}

	// you can use -> to easily dereference an item reference
	// for quick access checks before copying it out. this won't
	// throw if the item is invalid, but just returns the null item's
	// info.
	inline const gitem_t *operator->() const
	{
		return _ptr;
	}

	// can be converted to an actual item& implicitly
	inline operator const gitem_t&() const
	{
		return *_ptr;
	}

	// can be converted to an entity pointer implicitly
	inline operator const gitem_t*() const
	{
		return _ptr;
	}

	inline explicit operator bool() const
	{
		return has_value();
	}
};
