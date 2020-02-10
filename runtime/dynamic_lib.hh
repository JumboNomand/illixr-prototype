#ifndef DYNAMICLIB_HH__
#define DYNAMICLIB_HH__

#include <dlfcn.h>
#include <iostream>
#include <memory>
#include <string>

using void_ptr = std::unique_ptr<void, std::function<void(void*)>>;
/*
Usage:
    void* thing;
    void_ptr wrapped_thing = {thing, [](void* thing) {
        // destructor goes here.
    }}
    // wrapped_thing.get() returns underlying thing.
 */

class dynamic_lib {
private:
	dynamic_lib(void_ptr&& handle)
		: _m_handle{std::move(handle)}
	{ }

public:
	static dynamic_lib create(const std::string& path) {
		char* error;
		void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
		if ((error = dlerror()) != nullptr || !handle)
			throw std::runtime_error{
				"dlopen(): " + (error == nullptr ? "NULL" : std::string{error})};

		return dynamic_lib{void_ptr{handle, [](void* handle) {
			char* error;
			int ret = dlclose(handle);
			if ((error = dlerror()) != nullptr || ret)
				/* I can't throw exception in a destructor, but I should
				   tell someone about this. (this is also exceedingly
				   unlikely) */
				std::cerr
					<< "dlclose(): " << (error == nullptr ? "NULL" : error)
					<< std::endl;
		}}};
	}

	const void* operator[](const std::string& symbol_name) const {
		char* error;
		void* symbol = dlsym(_m_handle.get(), symbol_name.c_str());
		if ((error = dlerror()) != nullptr)
			throw std::runtime_error{
				"dlsym(): " + (error == nullptr ? "NULL" : std::string{error})};
		return symbol;
	}

	template <typename T>
	const T get(const std::string& symbol_name) const {
		const void* obj = (*this)[symbol_name];
		// return reinterpret_cast<const T>((*this)[symbol_name]);
		return (const T) obj;
	}

private:
	void_ptr _m_handle;
};

#endif
