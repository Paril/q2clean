module;

#include "types.h"

export module save;

import string;

export template<typename TFunc>
struct registered_save_function
{
	stringlit					name;
	TFunc						address;
	registered_save_function	*next;
	
	registered_save_function(stringlit name, TFunc address);
};

// linked list pointing to all savable functions.
export registered_save_function<void *> *registered_functions_head;

template<typename TFunc>
registered_save_function<TFunc>::registered_save_function(stringlit name, TFunc address) :
	name(name),
	address(address),
	next((registered_save_function<TFunc> *)registered_functions_head)
{
	registered_functions_head = (registered_save_function<void *> *)this;
}

// This type represents a function that can be serialized to disk.
// It is written out as a string and can be pulled back in across
// platforms and architectures.
export template<typename TFunc>
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

export template<typename T>
struct registered_save_data
{
	stringlit				name;
	T						&address;
	registered_save_data	*next;
	
	registered_save_data(stringlit name, T &address);
};

// linked list pointing to all savable functions.
export registered_save_data<void *> *registered_data_head;

export template<typename T>
registered_save_data<T>::registered_save_data(stringlit name, T &address) :
	name(name),
	address(address),
	next((registered_save_data<T> *)registered_data_head)
{
	registered_data_head = (registered_save_data<void *> *)this;
}

// This type represents a function that can be serialized to disk.
// It is written out as a string and can be pulled back in across
// platforms and architectures.
export template<typename TData>
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