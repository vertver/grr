/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_BASE_HPP_INCLUDED
#define GRR_BASE_HPP_INCLUDED
#include <grr/visit_struct/visit_struct.hpp>
#include <grr/visit_struct/visit_struct_intrusive.hpp>

#include <typeinfo>
#include <system_error>
#include <cstdint>

#ifdef __cpp_lib_reflection
#define GRR_TS_REFLECT
#elif defined(GRR_TS_REFLECT)
#error Unsupported compile for C++ reflection feature
#endif

#if defined(_MSC_VER) && __cplusplus == 199711L
#define GRR_CXX _MSVC_LANG 
#else
#define GRR_CXX __cplusplus 
#endif

#if GRR_CXX >= 202002L
#define GRR_CXX20_SUPPORT 1
#elif GRR_CXX >= 201703L
#else
#error Incompatible version of C++
#endif

#ifndef GRR_CONSTEXPR
#ifdef GRR_CXX20_SUPPORT
#define GRR_CONSTEXPR constexpr
#else
#define GRR_CONSTEXPR
#endif
#endif

#ifndef GRR_TYPE_ID
#define GRR_TYPE_ID std::uint64_t
#endif

#ifndef GRR_INVALID_SIZE
#define GRR_INVALID_SIZE static_cast<std::size_t>(-1)
#endif

#ifndef GRR_INVALID_ID
#define GRR_INVALID_ID static_cast<GRR_TYPE_ID>(-1)
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
#define GRR_HASH_SET std::unordered_set
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
	struct struct_context;
	class context;
	class type;

	template<typename T>
	using vector = GRR_VECTOR<T>;

	template<typename K, typename V>
	using hash_map = GRR_HASH_MAP<K, V>;

	template<typename K>
	using hash_set = GRR_HASH_SET<K>;

	using string = GRR_STRING;
	using string_view = GRR_STRING_VIEW;
	using type_id = GRR_TYPE_ID;

	struct member
	{
		member(const char* new_name, type_id new_id) : name(new_name), id(new_id) {}
		member(const string_view& new_name, type_id new_id) : name(new_name), id(new_id) {}
		member() = default;
		member(const member&) = default;
		member(member&&) = default;
		member& operator=(const member&) = default;
		member& operator=(member&&) = default;

		string_view name;
		type_id id;
	};

	struct type_context
	{
		type_id id;
		std::size_t size;
		string_view name;
		std::uint8_t user_memory[sizeof(void*) * 2];
		vector<std::pair<std::uint32_t, member>> members;
	};

	class context
	{
	private:
		using storage_map = hash_map<type_id, type_context>;
		hash_map<type_id, type_context> storage;

	public:
		bool contains(type_id id) const
		{
#ifdef GRR_CXX20
			return storage.contains(id);
#else
			return storage.find(id) != storage.end();
#endif
		}

		std::size_t size(type_id id) const
		{
			const auto& it = storage.find(id);
			if (it == storage.end()) {
				return static_cast<std::size_t>(-1);
			}

			return 0;
		}


	};

	constexpr
	auto
	generate_type_name()
	{
		return "";
	}

	template<typename T>
	constexpr
	auto
	type_name()
	{
		// #TODO: must be fixed
		return typeid(T).name();
	}

	template<typename T>
	constexpr 
	T
	binhash(const char* str)
	{
		std::uint32_t counter = 0;
		T hash = T(-1);
		while (*str != '\0') {
			hash *= 0x21;
			hash += str[counter];
			counter++;
		}

		return hash;
	}

	constexpr
	type_id
	obtain_id(const string_view& name)
	{
		return binhash<type_id>(name.data());
	}

	template<typename T>
	constexpr
	type_id
	obtain_id()
	{
		return obtain_id(type_name<T>());
	}

	inline
	std::size_t
	size(const context& current_context, type_id id)
	{
		return current_context.size(id);
	}

	inline
	bool
	contains(const context& current_context, type_id id)
	{
		return current_context.contains(id);
	}

	template<typename T>
	constexpr
	bool
	contains(const context& current_context)
	{
		return current_context.contains(obtain_id<T>());
	}

	template<typename T>
	constexpr
	bool
	size(const context& current_context)
	{
		return current_context.size(obtain_id<T>());
	}

	enum class errors : int
	{
		invalid_argument,
		out_of_range
	};

	struct error_category : public std::error_category
	{
		std::string message(int c) const
		{
			static const char* err_msg[] =
			{
				"Invalid argument",
				"Out of range"
			};

			return err_msg[c];
		}

		const char* name() const noexcept { return "GRR Error code"; }
		const static error_category& get()
		{
			const static error_category category_const;
			return category_const;
		}
	};

	inline
	std::error_code
	make_error_code(errors e)
	{
		return std::error_code(static_cast<int>(e), error_category::get());
	}
	
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

		type(const context& in_context)
			: current_context(&in_context), tname(generate_type_name()), tid(obtain_id(tname)) {}

		type(const context& in_context, const char* type_name)
			: current_context(&in_context), tname(type_name), tid(obtain_id(tname)) {}

		type(const context& in_context, const string_view& type_name)
			: current_context(&in_context), tname(type_name), tid(obtain_id(tname)) {}

	public:
#ifndef GRR_DISABLE_EXCEPTIONS
		template<typename T>
		void emplace(const char* member_name)
		{
			type_id id = obtain_id<T>();
			if (!grr::contains(*current_context, id)) {
				throw new std::invalid_argument("unregistered type id");
			}

			members.emplace_back(std::move(member(member_name, id)));
		}

		template<typename T>
		void emplace(const string_view& member_name)
		{
			type_id id = obtain_id<T>();
			if (!grr::contains(*current_context, id)) {
				throw new std::invalid_argument("unregistered type id");
			}

			members.emplace_back(std::move(member(member_name, id)));
		}

		void emplace(member&& move_member)
		{
			if (!grr::contains(*current_context, move_member.id)) {
				throw new std::invalid_argument("unregistered type id");
			}

			members.emplace_back(move_member);
		}

		void emplace(const member& new_member)
		{
			if (!grr::contains(*current_context, new_member.id)) {
				throw new std::invalid_argument("unregistered type id");
			}

			members.emplace_back(new_member);
		}
#endif
		template<typename T>
		void emplace(const char* member_name, std::error_code& err)
		{
			type_id id = obtain_id<T>();
			if (!grr::contains(*current_context, id)) {
				err = make_error_code(errors::invalid_argument);
				return;
			}

			members.emplace_back(std::move(member(member_name, id)));
		}

		template<typename T>
		void emplace(const string_view& member_name, std::error_code& err)
		{
			type_id id = obtain_id<T>();
			if (!grr::contains(*current_context, id)) {
				err = make_error_code(errors::invalid_argument);
				return;
			}

			members.emplace_back(std::move(member(member_name, id)));
		}

		void emplace(member&& move_member, std::error_code& err)
		{
			if (!grr::contains(*current_context, move_member.id)) {
				err = make_error_code(errors::invalid_argument);
				return;
			}

			members.emplace_back(move_member);
		}

		void emplace(const member& new_member, std::error_code& err)
		{
			if (!grr::contains(*current_context, new_member.id)) {
				err = make_error_code(errors::invalid_argument);
				return;
			}

			members.emplace_back(new_member);
		}

	private:
		bool member_erase(const char* member_name)
		{
			const std::size_t name_size = std::strlen(member_name);
			for (auto it = members.begin(); it != members.end(); it++) {
				const auto& member_elem = *it;
				std::size_t member_name_size = std::strlen(member_elem.name.data());
				if (name_size != member_name_size) {
					continue;
				}

				if (!std::strncmp(member_name, member_elem.name.data(), name_size)) {
					members.erase(it);
					return true;
				}
			}

			return false;
		}

		bool member_erase(std::size_t idx)
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
			if (!member_erase(idx)) {
				throw new std::invalid_argument("invalid index");
			}
		}

		void erase(const char* member_name)
		{
			if (!member_erase(member_name)) {
				throw new std::invalid_argument("invalid member name");
			}
		}	
		
		void erase(const string_view& member_name)
		{
			if (!member_erase(member_name.data())) {
				throw new std::invalid_argument("invalid member name");
			}
		}
#endif

		void erase(std::size_t idx, std::error_code& err) noexcept
		{
			if (!member_erase(idx)) {
				err = make_error_code(errors::invalid_argument);
			}
		}

		void erase(const char* member_name, std::error_code& err) noexcept
		{
			if (!member_erase(member_name)) {
				err = make_error_code(errors::invalid_argument);
			}
		}	
		
		void erase(const string_view& member_name, std::error_code& err) noexcept
		{
			if (!member_erase(member_name.data())) {
				err = make_error_code(errors::invalid_argument);
			}
		}

	public:
		void name(const char* new_name)
		{
			tname = new_name;
			tid = grr::obtain_id(new_name);
		}

	public:
		GRR_CONSTEXPR const string_view& name() const noexcept
		{
			return tname;
		}

		GRR_CONSTEXPR type_id id() const noexcept
		{
			return tid;
		}

		GRR_CONSTEXPR std::size_t count() const noexcept
		{
			return members.size();
		}

		GRR_CONSTEXPR std::size_t size() const noexcept
		{
			std::size_t type_size = 0;
			for (const auto& elem : members) {
				type_size += grr::size(*current_context, elem.id);
			}

			return type_size;
		}
	};

}

#endif