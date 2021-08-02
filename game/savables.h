#pragma once

#include "config.h"
#include "lib/protocol.h"

#ifdef SAVING

#include <type_traits>
#include "lib/types.h"
#include "lib/string.h"

template<typename TFunc>
struct registered_save_function
{
	stringlit					name;
	TFunc						address;
	registered_save_function	*next;

	registered_save_function(stringlit name, TFunc address);
};

// linked list pointing to all savable functions.
extern registered_save_function<void *> *registered_functions_head;

template<typename TFunc>
registered_save_function<TFunc>::registered_save_function(stringlit name, TFunc address) :
	name(name),
	address(address),
	next((registered_save_function<TFunc> *)registered_functions_head)
{
	registered_functions_head = (registered_save_function<void *> *)this;
}

template<typename T>
struct registered_save_data
{
	stringlit				name;
	T &address;
	registered_save_data *next;

	registered_save_data(stringlit name, T &address);
};

// linked list pointing to all savable functions.
extern registered_save_data<void *> *registered_data_head;

template<typename T>
registered_save_data<T>::registered_save_data(stringlit name, T &address) :
	name(name),
	address(address),
	next((registered_save_data<T> *)registered_data_head)
{
	registered_data_head = (registered_save_data<void *> *)this;
}

// Fetch the savable name for the given identifier
#define SAVABLE(n) \
	n##_savable

// Register a function as savable; this must be done in a source file.
#define REGISTER_SAVABLE_FUNCTION(func) \
	registered_save_function<decltype(&func)> SAVABLE(func) = { #func, func }

// Forward-declare a function as savable; this is done in headers, generally, to
// expose a _savable to outside of its source file.
#define DECLARE_SAVABLE_FUNCTION(func) \
	extern registered_save_function<decltype(&func)> SAVABLE(func)

// Register a function as savable; this must be done in a source file.
#define REGISTER_SAVABLE_DATA(data) \
	registered_save_data<decltype(data)> SAVABLE(data) = { #data, data }

// Forward-declare a function as savable; this is done in headers, generally, to
// expose a _savable to outside of its source file.
#define DECLARE_SAVABLE_DATA(func) \
	extern registered_save_data<decltype(data)> SAVABLE(data)

// This type represents a function that can be serialized to disk.
// It is written out as a string and can be pulled back in across
// platforms and architectures.
template<typename TFunc>
struct savable_function
{
	const registered_save_function<TFunc> *func;

	savable_function(const registered_save_function<TFunc> &func) : func(&func)
	{
	}

	savable_function(nullptr_t) : func(nullptr)
	{
	}

	savable_function() : func(nullptr)
	{
	}

	template<typename... Args>
	std::invoke_result_t<TFunc, Args...> operator()(Args&&... args)
	{
		if constexpr (std::is_void_v<std::invoke_result_t<TFunc, Args...>>)
			func->address(std::forward<Args>(args)...);
		else
			return func->address(std::forward<Args>(args)...);
	}

	explicit operator bool() const { return func; }
	explicit operator TFunc() const { return func ? func->address : nullptr; }

	bool operator==(const savable_function &f) const { return f.func == func; }
	bool operator!=(const savable_function &f) const { return f.func != func; }

	bool operator==(TFunc f) const { return f == (TFunc) *this; }
	bool operator!=(TFunc f) const { return f != (TFunc) *this; }
};

// This type represents a function that can be serialized to disk.
// It is written out as a string and can be pulled back in across
// platforms and architectures.
template<typename TData>
struct savable_data
{
	const registered_save_data<TData> *data;

	savable_data(const registered_save_data<TData> &data) : data(&data)
	{
	}

	savable_data(const registered_save_data<TData> *data) : data(data)
	{
	}

	savable_data() : data(nullptr)
	{
	}

	TData *operator->() const { return &data->address; }
	TData *operator*() const { return &data->address; }

	operator TData *() const { return data ? &data->address : nullptr; }

	explicit operator bool() const { return data; }

	bool operator==(const savable_data &f) const { return f.data == data; }
	bool operator!=(const savable_data &f) const { return f.data != data; }
};

void WriteGame(stringlit filename, qboolean autosave);

void ReadGame(stringlit filename);

void WriteLevel(stringlit filename);

void ReadLevel(stringlit filename);

#else

// Saving disabled; nop the macros and make the template just a simple passthrough

#define SAVABLE(n) n

#define REGISTER_SAVABLE_FUNCTION(func)
#define DECLARE_SAVABLE_FUNCTION(func)
#define REGISTER_SAVABLE_DATA(data)
#define DECLARE_SAVABLE_DATA(data)

template<typename TFunc>
using savable_function = TFunc;

template<typename TData>
using savable_data = TData *;

#include "lib/string.h"
#include "lib/protocol.h"

constexpr void WriteGame(stringlit, qboolean) { }

constexpr void ReadGame(stringlit) { }

constexpr void WriteLevel(stringlit) { }

constexpr void ReadLevel(stringlit) { }

#endif