#pragma once 

import <numeric>;
import <new>;

// memory tag
enum mem_tag : int32_t
{
	TAG_GAME = 765
};

namespace internal
{
	void *alloc(size_t len, mem_tag tag);
	void free(void *ptr);
	bool is_ready();
}

// a game_allocator directs memory for STL allocations over to
// the engine.
template<typename T>
class game_allocator
{
public:
	using value_type = T;
	using size_type = uint32_t;

	game_allocator() noexcept = default;

	template <class U>
	game_allocator(game_allocator<U> const &alloc [[maybe_unused]] ) noexcept :
		game_allocator()
	{
	}

	[[nodiscard]] value_type *allocate(size_t num)
	{
		if (num > std::numeric_limits<size_type>::max() / sizeof(value_type))
			throw std::bad_alloc();

		int32_t *ptr;

		if (!internal::is_ready())
		{
			ptr = (int32_t *) calloc(1, (num * sizeof(value_type)) + sizeof(int32_t));
			*ptr = 0;
		}
		else
		{
			ptr = (int32_t *) internal::alloc((num * sizeof(value_type)) + sizeof(int32_t), TAG_GAME);
			*ptr = 1;
		}

		return (value_type *) (ptr + 1);
	}

	void deallocate(value_type *ptr, size_t num [[maybe_unused]] ) noexcept
	{
		int32_t *p = ((int32_t *) ptr) - 1;

		if (!*p)
			free(p);
		else
			internal::free(p);
	}
};

template <class T, class U>
bool operator==(const game_allocator<T> &, const game_allocator<U> &) { return true; }
template <class T, class U>
bool operator!=(const game_allocator<T> &, const game_allocator<U> &) { return false; }