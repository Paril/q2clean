#pragma once

#include "types.h"

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

// Register a function as savable; this must be done in a source file.
#define REGISTER_SAVABLE_FUNCTION(func) \
	registered_save_function<decltype(&func)> func##_savable = { #func, func };

// Forward-declare a function as savable; this is done in headers, generally, to
// expose a _savable to outside of its source file.
#define DECLARE_SAVABLE_FUNCTION(func) \
	extern registered_save_function<decltype(&func)> func##_savable;

// This type represents a function that can be serialized to disk.
// It is written out as a string and can be pulled back in across
// platforms and architectures.
template<typename TFunc>
struct savable_function
{
	const registered_save_function<TFunc>	*func;

	savable_function() : func(nullptr)
	{
	}

	savable_function(nullptr_t) : savable_function()
	{
	}

	savable_function(const registered_save_function<TFunc> &func) :
		func(&func)
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
	
	bool operator==(const savable_function &f) const { return f.func == func; }
	bool operator!=(const savable_function &f) const { return f.func != func; }
};


template<typename T>
struct registered_save_data
{
	stringlit				name;
	T						&address;
	registered_save_data	*next;
	
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

// Register a function as savable; this must be done in a source file.
#define REGISTER_SAVABLE_DATA(data) \
	registered_save_data<decltype(data)> data##_savable = { #data, data };

// Forward-declare a function as savable; this is done in headers, generally, to
// expose a _savable to outside of its source file.
#define DECLARE_SAVABLE_DATA(func) \
	extern registered_save_data<decltype(data)> data##_savable;

// This type represents a function that can be serialized to disk.
// It is written out as a string and can be pulled back in across
// platforms and architectures.
template<typename TData>
struct savable_data
{
	const registered_save_data<TData>	*data;

	savable_data() : data(nullptr)
	{
	}

	savable_data(nullptr_t) : savable_data()
	{
	}

	savable_data(const registered_save_data<TData> &data) :
		data(&data)
	{
	}
	
	TData *operator->() const { return &data->address; }
	TData *operator*() const { return &data->address; }
	
	explicit operator bool() const { return data; }
	operator TData *() const { return data ? &data->address : nullptr; }
	
	bool operator==(const savable_data &f) const { return f.data == data; }
	bool operator!=(const savable_data &f) const { return f.data != data; }
};