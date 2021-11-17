#pragma once

#include "config.h"
#include "lib/protocol.h"
#include "lib/std.h"

template<typename T>
struct is_function_pointer
{
	static constexpr bool value = std::is_pointer<T>::value ? std::is_function<typename std::remove_pointer<T>::type>::value : false;
};

template<typename T>
constexpr bool is_function_pointer_v = is_function_pointer<T>::value;

#ifdef SAVING

#include "lib/types.h"
#include "lib/string.h"

// savable piece of data
template<typename T>
struct registered_savable
{
	static constexpr bool function_ptr = is_function_pointer_v<T>;
	using address_type = std::conditional_t<function_ptr, T, const T &>;

	stringlit			name;
	address_type		address;
	registered_savable	*next;

	constexpr registered_savable(stringlit name, address_type address);
};

extern registered_savable<void *> *registered_data_head;
extern registered_savable<void(*)()> *registered_functions_head;

template<typename T>
constexpr registered_savable<T>::registered_savable(stringlit name, address_type address) :
	name(name),
	address(address),
	next(function_ptr ? (registered_savable *) registered_functions_head : (registered_savable *) registered_data_head)
{
	if constexpr(function_ptr)
		registered_functions_head = (decltype(registered_functions_head)) this;
	else
		registered_data_head = (decltype(registered_data_head)) this;
}

// Fetch the savable name for the given identifier
#define SAVABLE(n) \
	n##_savable

template<typename T>
using savable_type = std::conditional_t<std::is_function_v<T>, T *, T>;

// Register something as savable; this must be done in a source file.
// If you're not exporting the savable, use REGISTER_STATIC_SAVABLE
#define REGISTER_SAVABLE(T) \
	registered_savable<savable_type<decltype(T)>> SAVABLE(T) = { #T, T }

#define REGISTER_STATIC_SAVABLE(T) \
	static REGISTER_SAVABLE(T)

// Forward-declare a function as savable; this is done in headers, generally, to
// expose a savable to outside of its source file.
#define DECLARE_SAVABLE(T) \
	extern registered_savable<savable_type<decltype(T)>> SAVABLE(T)

// This type represents a function or data that can be serialized to disk.
// It is written out as a string and can be pulled back in across
// platforms and architectures.
template<typename T>
struct savable
{
	static constexpr bool is_function = is_function_pointer_v<T>;
	using ptr_type = std::conditional_t<is_function, T, T *>;

	const registered_savable<T> *registry;

	constexpr savable(const registered_savable<T> &registry) : registry(&registry)
	{
	}

	constexpr savable(const registered_savable<T> *registry) : registry(registry)
	{
	}

	constexpr savable(nullptr_t) : registry(nullptr)
	{
	}

	constexpr savable() : registry(nullptr)
	{
	}

	explicit operator bool() const { return registry; }

	template<typename... Args>
	std::invoke_result_t<T, Args...> operator()(Args&&... args) requires is_function
	{
		if constexpr (std::is_void_v<std::invoke_result_t<T, Args...>>)
			registry->address(std::forward<Args>(args)...);
		else
			return registry->address(std::forward<Args>(args)...);
	}

	constexpr operator ptr_type() const
	{
		if constexpr (is_function)
			return registry ? registry->address : nullptr;
		else
			return registry ? &registry->address : nullptr;
	}

	ptr_type operator->() const requires (!is_function) { return &registry->address; }
	ptr_type operator*() const requires (!is_function) { return &registry->address; }

	bool operator==(const savable &f) const { return f.registry == registry; }
	bool operator!=(const savable &f) const { return f.registry != registry; }

	bool operator==(const T *f) const { return f == (ptr_type) *this; }
	bool operator!=(const T *f) const { return f != (ptr_type) *this; }

	bool operator==(const T &f) const { return f == (ptr_type) *this; }
	bool operator!=(const T &f) const { return f != (ptr_type) *this; }
};

void WriteGame(stringlit filename, qboolean autosave);

void ReadGame(stringlit filename);

void WriteLevel(stringlit filename);

void ReadLevel(stringlit filename);

#else

// Saving disabled; nop the macros and make the template just a simple passthrough

#define SAVABLE(n) n

#define REGISTER_SAVABLE(func)
#define REGISTER_STATIC_SAVABLE(func)
#define DECLARE_SAVABLE(func)

template<typename T>
using savable = std::conditional_t<std::is_function_v<T>, T *, std::conditional_t<is_function_pointer_v<T>, T, T *>>;

#include "lib/string.h"
#include "lib/protocol.h"
#include "lib/gi.h"

inline void WriteGame(stringlit, qboolean)
{
	gi.dprint("Saving is not enabled in this mod.\n");
}

constexpr void ReadGame(stringlit) { }

inline void WriteLevel(stringlit)
{
	gi.dprint("Saving is not enabled in this mod.\n");
}

constexpr void ReadLevel(stringlit) { }

#endif

