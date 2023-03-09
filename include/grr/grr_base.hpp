/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_BASE_HPP_INCLUDED
#define GRR_BASE_HPP_INCLUDED
#include <typeinfo>
#include <system_error>
#include <cstdint>
#include <utility>

#ifdef GRR_PREDECLARE_FIELDS
#include <pfr.hpp>
#include <visit_struct/visit_struct.hpp>
#include <visit_struct/visit_struct_intrusive.hpp>
#endif

#if defined(GRR_TS_REFLECT) && !defined(__cpp_lib_reflection)
#error Unsupported compile for C++ reflection feature
#endif

#ifndef GRR_USER_TYPES
#define GRR_USER_TYPES
#endif

#if defined(_MSC_VER) && __cplusplus == 199711L
#define GRR_CXX _MSVC_LANG 
#else
#define GRR_CXX __cplusplus 
#endif

#if GRR_CXX >= 202002L
#define GRR_CXX20_SUPPORT 1
#elif GRR_CXX < 201703L
#error Incompatible version of C++
#endif

#ifndef GRR_CONSTEXPR
#ifdef GRR_CXX20_SUPPORT
#define GRR_CONSTEXPR constexpr
#else
#define GRR_CONSTEXPR
#endif
#endif

#ifndef GRR_TYPE_NAME
#ifdef _MSC_VER
#define GRR_TYPE_NAME __FUNCSIG__
#else
#define GRR_TYPE_NAME __PRETTY_FUNCTION__
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
	struct type_context;
	class context;
	class type_declaration;

	template<typename T>
	using vector = GRR_VECTOR<T>;

	template<typename K, typename V>
	using hash_map = GRR_HASH_MAP<K, V>;

	template<typename K>
	using hash_set = GRR_HASH_SET<K>;

	using string = GRR_STRING;
	using string_view = GRR_STRING_VIEW;
	using type_id = GRR_TYPE_ID;

	using ptr_pair = std::pair<std::uint64_t, std::pair<void*, type_id>>;
	using const_ptr_pair = std::pair<std::uint64_t, std::pair<const void*, type_id>>;

#ifndef GRR_TYPES
#define GRR_TYPES \
	void*, \
	char, \
	char*, \
	std::int8_t, \
	std::int16_t, \
	std::int32_t, \
	std::int64_t, \
	std::uint8_t, \
	std::uint16_t, \
	std::uint32_t, \
	std::uint64_t, \
	std::int8_t*, \
	std::int16_t*, \
	std::int32_t*, \
	std::int64_t*, \
	std::uint8_t*, \
	std::uint16_t*, \
	std::uint32_t*, \
	std::uint64_t*, \
	float, \
	double, \
	float*, \
	double*, \
	\
	grr::ptr_pair, \
	grr::ptr_pair*, \
	grr::const_ptr_pair, \
	grr::const_ptr_pair*, \
	grr::string, \
	grr::string*, \
	grr::string_view, \
	grr::string_view* \
	GRR_USER_TYPES
#endif
	struct field
	{
		field(const char* new_name, type_id new_id, std::size_t new_offset)
			: name(new_name), id(new_id), offset(new_offset) {}

		field(const string_view& new_name, type_id new_id, std::size_t new_offset)
			: name(new_name), id(new_id), offset(new_offset) {}

		field() = default;
		field(const field&) = default;
		field(field&&) = default;
		field& operator=(const field&) = default;
		field& operator=(field&&) = default;

		std::size_t offset;
		type_id id;
		string name;
	};

	struct type_context
	{
		std::size_t size;
		string display_name;
		string real_name;
		vector<field> fields;
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

			return it->second.size;
		}

		const type_context& obtain(type_id id) const
		{
			return storage.at(id);
		}

		storage_map::const_iterator begin() const
		{
			return storage.begin();
		}
		
		storage_map::const_iterator end() const
		{
			return storage.end();
		}

		void rename(type_id id, const char* new_name)
		{
			const auto& it = storage.find(id);
			if (it == storage.end()) {
				// #TODO: error handling here
				return;
			}

			it->second.display_name = new_name;
		}

		void rename(type_id id, const string_view& new_name)
		{
			const auto& it = storage.find(id);
			if (it == storage.end()) {
				// #TODO: error handling here
				return;
			}

			it->second.display_name = string(new_name.begin(), new_name.end());
		}

		void add(type_id id, const type_context& type)
		{
			storage.emplace(std::make_pair(id, type));
		}

		void add(type_id id, type_context&& type)
		{
			storage.emplace(std::make_pair(id, type));
		}
	};

	// https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/
	template<typename T>
	constexpr string_view type_name(int unused /* hack for newer versions of MSVC */)
	{
		(void)(unused);

#if defined(__clang__)
		constexpr string_view prefix = "[T = ";
		constexpr string_view suffix = "]";
		constexpr string_view function = __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
		constexpr string_view prefix = "with T = ";
		constexpr string_view suffix = "]";
		constexpr string_view function = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
		constexpr string_view prefix = "type_name<";
		constexpr string_view suffix = ">(int)";
		constexpr string_view function = __FUNCSIG__;
#endif
		constexpr std::size_t start = function.find(prefix) + prefix.size();
		constexpr std::size_t end = function.rfind(suffix);

		static_assert(start < end);
		return function.substr(start, (end - start));
	}

	template<typename T>
	constexpr string_view type_name()
	{
		return grr::type_name<std::remove_cv_t<T>>(0);
	}

	template<typename T>
	constexpr T binhash(const string_view& str)
	{
		T hash = T(5381);
		for (const char& sym : str) {
			hash *= 0x21;
			hash += sym;
		}

		return hash;
	}

	template<typename T>
	constexpr T binhash(const char* str)
	{
		return *str != '\0' ? static_cast<unsigned int>(*str) + 33 * binhash<T>(str + 1) : 5381;
	}

	constexpr type_id obtain_id(const char* name)
	{
		return binhash<type_id>(name);
	}	
	
	constexpr type_id obtain_id(const string_view& name)
	{
		return binhash<type_id>(name);
	}

	template<typename T>
	constexpr type_id obtain_id()
	{
		using CT = std::remove_reference_t<std::remove_cv_t<T>>;
		return obtain_id(type_name<CT>());
	}

	inline std::size_t size(const context& current_context, type_id id)
	{
		return current_context.size(id);
	}

	inline bool contains(const context& current_context, type_id id)
	{
		return current_context.contains(id);
	}

	inline void rename(context& current_context, type_id id, const char* new_name)
	{
		current_context.rename(id, new_name);
	}
	
	inline void rename(context& current_context, type_id id, const string_view& new_name)
	{
		current_context.rename(id, new_name);
	}

	template<typename T>
	constexpr void rename(context& current_context, const string_view& new_name)
	{
		using CT = std::remove_reference_t<std::remove_cv_t<T>>;
		current_context.rename(obtain_id<CT>(), new_name);
	}

	template<typename T>
	constexpr void rename(context& current_context, const char* new_name)
	{
		using CT = std::remove_reference_t<std::remove_cv_t<T>>;
		current_context.rename(obtain_id<CT>(), new_name);
	}

	template<typename T>
	constexpr bool contains(const context& current_context)
	{
		using CT = std::remove_reference_t<std::remove_cv_t<T>>;
		return current_context.contains(obtain_id<CT>());
	}

	template<typename T>
	constexpr bool size(const context& current_context)
	{
		using CT = std::remove_reference_t<std::remove_cv_t<T>>;
		return current_context.size(obtain_id<CT>());
	}

	enum class errors : int
	{
		invalid_argument,
		unregistered_id,
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

	inline std::error_code make_error_code(errors e)
	{
		return std::error_code(static_cast<int>(e), error_category::get());
	}

	template<typename T>
	static constexpr void visit_static_once(auto data, const char* name, type_id id, bool& called, auto&& func)
	{
		// #TODO: poor optimized, need to rework this one
		constexpr type_id current_id = grr::obtain_id<T>();
		if (!called && current_id == id) {
			if constexpr (std::is_const_v<std::remove_pointer_t<decltype(data)>>) {
				func(*reinterpret_cast<const T*>(data), name);
			} else {
				func(*reinterpret_cast<T*>(data), name);
			}

			called = true;
		}
	}

	template<typename... Types>
	static constexpr bool visit_static(auto data, const char* name, type_id id, auto&& func)
	{
		bool called = false;
		(visit_static_once<Types>(data, name, id, called, func), ...);
		return called;
	}

	template<std::size_t recursion_level = 0>
	static constexpr void visit(const grr::context& context, auto data, type_id id, auto&& func)
	{
		if (!context.contains(id)) {
			throw new std::invalid_argument("unregistered type id");
		}

		const auto& type_info = context.obtain(id);
		for (const auto& cfield : type_info.fields) {
			auto field_ptr = data;
			if constexpr (std::is_const_v<std::remove_pointer_t<decltype(data)>>) {
				field_ptr = static_cast<const char*>(data) + cfield.offset;
			} else {
				field_ptr = static_cast<char*>(data) + cfield.offset;
			}		
			
			type_id field_id = cfield.id;

			if constexpr (recursion_level > 0) {
				visit<recursion_level - 1>(context, field_ptr, field_id);
			} else {
				if (visit_static<GRR_TYPES>(field_ptr, cfield.name.data(), cfield.id, func)) {
					continue;
				}

				if (!context.contains(field_id)) {
					throw new std::invalid_argument("unregistered type id");
				}

				const auto& field_type = context.obtain(field_id);
				func(std::make_pair(field_type.size, std::make_pair(field_ptr, field_id)), cfield.name.data());
			}
		}
	}
	
	template<std::size_t recursion_levels = 0, typename T>
	static constexpr void visit(const grr::context& context, T& data, auto&& func)
{
		constexpr type_id id = grr::obtain_id<T>();
		if constexpr (std::is_const_v<T>) {
			grr::visit(context, reinterpret_cast<const void*>(&data), id, func);
		} else {
			grr::visit(context, reinterpret_cast<void*>(&data), id, func);
		}
	}

	struct type_declaration
	{
		const context* current_context;
		std::size_t size;
		string real_name;
		type_id id;
		vector<field> fields;

		type_declaration(type_declaration&) = delete;
		type_declaration(type_declaration&&) = default;

		type_declaration(const context& in_context, const char* type_name)
			: current_context(&in_context), real_name(type_name), id(obtain_id(real_name)), size(0) {}	
		
		type_declaration(const context& in_context, const string_view& type_name)
			: current_context(&in_context), real_name(type_name), id(obtain_id(real_name)), size(0) {}

		bool field_erase(const char* field_name)
		{
			const std::size_t name_size = std::strlen(field_name);
			for (auto it = fields.begin(); it != fields.end(); it++) {
				const auto& field_elem = *it;
				std::size_t field_name_size = std::strlen(field_elem.name.data());
				if (name_size != field_name_size) {
					continue;
				}

				if (!std::strncmp(field_name, field_elem.name.data(), name_size)) {
					fields.erase(it);
					return true;
				}
			}

			return false;
		}

		bool field_erase(std::size_t idx)
		{
			if (idx >= fields.size()) {
				return false;
			}

			fields.erase(fields.begin() + idx);
			return true;
		}

		template<typename T>
		void emplace(const char* field_name)
		{
			constexpr type_id current_id = obtain_id<T>();
			if (!grr::contains(*current_context, current_id)) {
				throw new std::invalid_argument("unregistered type id");
			}

			const std::size_t type_size = fields.empty() ? 0 : grr::size(*current_context, fields.back().id);
			const std::size_t offset = fields.empty() ? 0 : fields.back().offset + type_size;
			fields.emplace_back(std::move(field(field_name, current_id, offset)));
		}

		template<typename T>
		void emplace(const char* field_name, std::size_t offset)
		{
			constexpr type_id current_id = obtain_id<T>();
			if (!grr::contains(*current_context, current_id)) {
				throw new std::invalid_argument("unregistered type id");
			}

			fields.emplace_back(std::move(field(field_name, current_id, offset)));
		}

		void erase(std::size_t idx)
		{
			if (!field_erase(idx)) {
				throw new std::invalid_argument("invalid index");
			}
		}

		void erase(const char* field_name)
		{
			if (!field_erase(field_name)) {
				throw new std::invalid_argument("invalid field name");
			}
		}
	};

	inline void add_type(context& current_context, const type_declaration& type)
	{
		if (current_context.contains(type.id)) {
			string type_name = "type already exists [";
			type_name += type.real_name;
			type_name += "]";
			throw new std::invalid_argument(type_name);
		}

		type_context tcontext;
		tcontext.real_name = type.real_name;
		tcontext.display_name = type.real_name;
		tcontext.fields = type.fields;
		tcontext.size = type.size;
		current_context.add(type.id, std::move(tcontext));
	}

	template<typename T> 
	void add_type(context& current_context)
	{
		using CT = std::remove_cv_t<T>;

		type_declaration new_type = type_declaration(current_context, grr::type_name<T>());
#ifdef GRR_PREDECLARE_FIELDS
		if constexpr (std::is_class_v<CT> && pfr::is_implicitly_reflectable_v<CT, CT>) {
			CT val = {};
			static_assert(!visit_struct::traits::is_visitable<CT>::value);

			pfr::for_each_field(val, [&val, &new_type](auto& field) {
				std::ptrdiff_t offset = (std::ptrdiff_t)(&field) - (std::ptrdiff_t)(&val);

				grr::string temp_buffer;
				temp_buffer.resize(14);
				std::snprintf(temp_buffer.data(), 10, "%u", (std::uint32_t)offset);

				grr::string field_name = "var" + grr::string(temp_buffer);
				new_type.emplace<decltype(field)>(field_name.data(), offset);
				new_type.size += sizeof(field);
			});
		} else {
			new_type.size = sizeof(CT);
		}
#endif

		grr::add_type(current_context, new_type);
	}

	template<typename... Types>
	void add_types(context& current_context)
	{
		(add_type<Types>(current_context), ...);
	}

	inline context make_context()
	{
		context out_context;
		add_types<GRR_TYPES>(out_context);
		return out_context;
	}
}

#endif