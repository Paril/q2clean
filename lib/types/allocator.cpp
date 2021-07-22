#include "lib/types.h"
#include "lib/types/allocator.h"
#include "lib/gi.h"

void *internal::alloc(size_t len, mem_tag tag)
{
	return gi.TagMalloc((uint32_t) len, (uint32_t) tag);
}

void internal::free(void *ptr)
{
	gi.TagFree(ptr);
}

bool internal::is_ready()
{
	return gi.is_ready();
}
