#pragma once 

#include <numeric>
#include <new>

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
	extern uint32_t non_game_count, game_count;
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
			internal::non_game_count += num;
		}
		else
		{
			ptr = (int32_t *) internal::alloc((num * sizeof(value_type)) + sizeof(int32_t), TAG_GAME);
			internal::game_count += num;
		}

		if (!ptr)
			throw std::bad_alloc();

		*ptr = internal::is_ready();

		return (value_type *) (ptr + 1);
	}

	void deallocate(value_type *ptr, size_t num [[maybe_unused]] ) noexcept
	{
		int32_t *p = ((int32_t *) ptr) - 1;

		if (!*p)
		{
			free(p);
			internal::non_game_count -= num;
		}
		else
		{
			internal::free(p);
			internal::game_count -= num;
		}
	}
};

template <class T, class U>
bool operator==(const game_allocator<T> &, const game_allocator<U> &) { return true; }
template <class T, class U>
bool operator!=(const game_allocator<T> &, const game_allocator<U> &) { return false; }