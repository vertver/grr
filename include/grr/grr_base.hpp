/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_BASE_HPP_INCLUDED
#define GRR_BASE_HPP_INCLUDED
#include <visit_struct/visit_struct.hpp>
#include <visit_struct/visit_struct_intrusive.hpp>

#include <system_error>
#include <cstdint>

#ifndef GRR_TYPE_ID
#define GRR_TYPE_ID std::uint64_t
#endif

#ifndef GRR_DISABLE_EXCEPTIONS
#include <exception>
#endif

#ifndef GRR_VECTOR
#include <vector>
#define GRR_VECTOR std::vector
#endif

#ifndef GRR_HASH_MAP
#include <unordered_map>
#define GRR_HASH_MAP std::unordered_map
#endif

#ifndef GRR_HASH_SET
#include <unordered_set>
#define GRR_HASH_MAP std::unordered_set
#endif

#ifndef GRR_STRING
#include <string>
#define GRR_STRING std::string
#endif

#ifndef GRR_STRING_VIEW
#include <string_view>
#define GRR_STRING_VIEW std::string_view
#endif

namespace grr
{

struct context;
struct struct_context;

using vector = GRR_VECTOR;
using hash_map = GRR_HASH_MAP;
using hash_set = GRR_HASH_SET;
using string = GRR_STRING;
using string_view = GRR_STRING_VIEW;
using type_id = GRR_TYPE_ID;

enum class reflect_options : int
{
	disable_pre_declaration
};

enum class errors : int
{
    invalid_argument,
    out_of_range
};

inline
const char* 
error_message(int c)
{
    static const char* err_msg[] = 
    {
        "Invalid argument",
        "Out of range"
    };
 
    return err_msg[c];
}

struct error_category : public std::error_category
{
	std::string message(int c) const
    { 
        return error_message(c); 
    }
 
    const char* name() const { return "GRR Error code"; }
    const static error_category& get()
    {
        const static error_category category_const;
        return category_const;
    }
}

inline 
std::error_code
make_error_code(errors e)
{
	return std::error_code(static_cast<int>(e), error_category::get());
}

constexpr 
string_view 
generate_type_name()
{
	return "";
}

constexpr
type_id
get_type_id(const char* type_name)
{
	return 0;
}

template<typename T>
constexpr
get_type_id()
{
	return get_type_id(visit_struct::get_name<T>());
}

struct member
{
	member(const char* new_name, type_id new_id) : name(new_name), id(new_id) {}
	member() = default;
	member(member&&) = default;

	string_view name;
	type_id id;
};

class type
{
private:	
	const context* current_context;
	string_view tname;
	type_id tid;
	vector<member> members;

public:
	type(type&) = delete;
	type(type&&) = default;
	
	type(const context* in_context) 
		: current_context(in_context), tname(generate_type_name()), tid(get_type_id(tname.data())) {}
	
	type(const context* in_context, const char* type_name) 
		: current_context(in_context), tname(type_name), tid(get_type_id(tname.data())) {}
	
	type(const context* in_context, const string_view& type_name) 
		: current_context(in_context), tname(type_name), tid(get_type_id(tname.data())) {}

public:
	template<typename T>
	void emplace(const char* member_name)
	{
		type_id id = get_type_id<T>();
		if (!current_context->contains(type_id)) {
			throw new std::invalid_argument("unregistered type id");
		}

		members.emplace(std::move(member(member_name, id)));
	}

	void emplace(member&& move_member)
	{
		if (!current_context->contains(move_member.id)) {
			throw new std::invalid_argument("unregistered type id");
		}

		members.emplace(move_member);
	}

	void emplace(const member& new_member)
	{
		if (!current_context->contains(move_member.id)) {
			throw new std::invalid_argument("unregistered type id");
		}

		members.emplace(new_member);
	}

private:
	bool erase(const char* member_name)
	{
		const std::size_t name_size = std::strlen(member_name);
		for (auto it = members.begin(); it != members.end(); it++) {
			const auto& member_elem = *it;
			std::size_t member_name_size = std::strlen(member_elem.name);
			if (name_size != member_name_size) {
				continue;
			}

			if (!std::strncmp(member_name, member_elem.name, name_size)) {
				members.erase(it);
				return true;
			}
		}

		return false;
	}

	bool erase(std::size_t idx)
	{
		if (idx >= members.size()) {
			return false;
		}

		members.erase(members.begin() + idx);
		return true;
	}

public:
#ifndef GRR_DISABLE_EXCEPTIONS
	void erase(std::size_t idx)
	{
		if (!erase(idx)) {
			throw new std::invalid_argument("invalid index");
		}
	}

	void erase(const char* member_name)
	{
		if (!erase(member_name)) {
			throw new std::invalid_argument("invalid member name");
		}
	}
#endif

	void erase(std::size_t idx, std::error_code& err) noexcept
	{
		if (!erase(idx)) {
			err = make_error_code(errors::invalid_argument);
		}
	}

	void erase(const char* member_name, std::error_code& err) noexcept
	{
		if (!erase(member_name)) {
			err = make_error_code(errors::invalid_argument);
		}
	}

public:
	void name(const char* new_name)
	{
		tname = new_name;
	}

public:
	const string_view& name() const noexcept 
	{
		return tname;
	}

	type_id id() const noexcept
	{
		return tid;
	}

	std::size count() const noexcept
	{
		return members.size();
	}

	std::size_t size() const noexcept
	{
		std::size_t type_size = 0;
		for (const auto& elem : members) {
			type_size += grr::size(members.id);
		}

		return type_size;
	}
};

struct struct_context
{

};

struct context
{
	hash_map<string_view, struct_context> storage;
	
};

}

#endif