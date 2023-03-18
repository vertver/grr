/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_BASE_HPP_INCLUDED
#define GRR_BASE_HPP_INCLUDED
#include <grr/detail/name_parser.hpp>

namespace grr
{
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
		type_id base_type;
		std::size_t size;
		string name;
		string platform_name;
		vector<field> fields;
	};

	class context
	{
	private:
		using storage_map = hash_map<type_id, type_context>;
		hash_map<type_id, type_context> storage;

	public:
		type_context& at(type_id id)
		{
			return storage.at(id);
		}

		const type_context& at(type_id id) const
		{
			return storage.at(id);
		}

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

			it->second.name = new_name;
		}

		void rename(type_id id, const string_view& new_name)
		{
			const auto& it = storage.find(id);
			if (it == storage.end()) {
				// #TODO: error handling here
				return;
			}

			it->second.name = string(new_name.begin(), new_name.end());
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

	template<typename T>
	constexpr string_view type_name()
	{
		return grr::detail::compiler_type_name<T>(0);
	}

	const char* type_name(const context& current_context, type_id id)
	{
		if (!current_context.contains(id)) {
			return "";
		}

		return current_context.at(id).name.c_str();
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
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		return obtain_id(type_name<ClearType>());
	}

	inline std::size_t size(const context& current_context, type_id id)
	{
		return current_context.size(id);
	}

	inline bool contains(const context& current_context, type_id id)
	{
		return current_context.contains(id);
	}

	inline void rename(context& current_context, type_id id, std::size_t field_idx, const char* new_name)
	{
		current_context.at(id).fields.at(field_idx).name = new_name;
	}

	inline void rename(context& current_context, type_id id, std::size_t field_idx, const string_view& new_name)
	{
		current_context.at(id).fields.at(field_idx).name = string(new_name.begin(), new_name.end());
	}

	inline void rename(context& current_context, type_id id, const char* new_name)
	{
		current_context.rename(id, new_name);
	}
	
	inline void rename(context& current_context, type_id id, const string_view& new_name)
	{
		current_context.rename(id, new_name);
	}

	inline std::size_t offset(context& current_context, type_id id, std::size_t field_idx)
	{
		return current_context.at(id).fields.at(field_idx).offset;
	}

	inline type_id base_type(context& current_context, type_id id)
	{
		return current_context.at(id).base_type;
	}

	template<typename T>
	constexpr type_id base_type()
	{
		return obtain_id<std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<T>>>>();
	}

	template<typename T>
	constexpr void rename(context& current_context, std::size_t field_idx, const string_view& new_name)
	{
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		current_context.at(obtain_id<ClearType>()).fields.at(field_idx).name = string(new_name.begin(), new_name.end());
	}

	template<typename T>
	constexpr void rename(context& current_context, const string_view& new_name)
	{
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		current_context.rename(obtain_id<ClearType>(), new_name);
	}
	 
	template<typename T>
	constexpr void rename(context& current_context, const char* new_name)
	{
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		current_context.rename(obtain_id<ClearType>(), new_name);
	}

	template<typename T>
	constexpr bool contains(const context& current_context)
	{
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		return current_context.contains(obtain_id<ClearType>());
	}

	template<typename T>
	constexpr bool size(const context& current_context)
	{
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		return current_context.size(obtain_id<ClearType>());
	}

	template<typename T>
	constexpr std::size_t offset(context& current_context, std::size_t field_idx)
	{
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		return current_context.at(obtain_id<ClearType>()).fields.at(field_idx).offset;
	}

	namespace detail
	{
		template<typename T>
		static constexpr void visit_static_once(auto data, const char* name, type_id id, bool& called, auto&& func)
		{
			using ClearDataType = std::remove_pointer_t<decltype(data)>;

			// #TODO: poor optimized, need to rework this one
			if constexpr (!std::is_same_v<T, void>) {
				constexpr type_id current_id = grr::obtain_id<T>();
				if (!called && current_id == id) {
					if constexpr (std::is_const_v<ClearDataType>) {
						func(*reinterpret_cast<const T*>(data), name);
					} else {
						func(*reinterpret_cast<T*>(data), name);
					}

					called = true;
				}
			}
			
			constexpr type_id current_ptr_id = grr::obtain_id<T*>();
			if (!called && current_ptr_id == id) {
				if constexpr (std::is_const_v<ClearDataType>) {
					func(*reinterpret_cast<const T**>(reinterpret_cast<size_t>(data)), name);
				} else {
					func(*reinterpret_cast<T**>(reinterpret_cast<size_t>(data)), name);
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
			const type_id field_id = cfield.id;
			if constexpr (std::is_const_v<std::remove_pointer_t<decltype(data)>>) {
				field_ptr = static_cast<const char*>(data) + cfield.offset;
			} else {
				field_ptr = static_cast<char*>(data) + cfield.offset;
			}		
			
			if constexpr (recursion_level > 0) {
				visit<recursion_level - 1>(context, field_ptr, field_id);
			} else {
				if (detail::visit_static<GRR_TYPES>(field_ptr, cfield.name.data(), cfield.id, func)) {
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
		bool aggregate = false;
		std::size_t size;
		string name;
		type_id id;
		vector<field> fields;

		type_declaration() = delete;
		type_declaration(type_declaration&) = delete;
		type_declaration(const type_declaration&) = delete;
		type_declaration(type_declaration&&) = default;

		type_declaration(const context& in_context, const char* type_name)
			: current_context(&in_context), name(type_name), id(obtain_id(name)), size(0) {}
		
		type_declaration(const context& in_context, const string_view& type_name)
			: current_context(&in_context), name(type_name), id(obtain_id(name)), size(0) {}	
		
		type_declaration(const context& in_context, const char* type_name, std::size_t new_size)
			: current_context(&in_context), name(type_name), id(obtain_id(name)), size(new_size) {}
		
		type_declaration(const context& in_context, const string_view& type_name, std::size_t new_size)
			: current_context(&in_context), name(type_name), id(obtain_id(name)), size(new_size) {}

		bool field_erase(const char* field_name)
		{
			const std::size_t name_size = std::strlen(field_name);
			for (auto it = fields.begin(); it != fields.end(); it++) {
				const auto& field_elem = *it;
				const std::size_t field_name_size = std::strlen(field_elem.name.data());
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

			const std::size_t offset = fields.empty() ? 0 : fields.back().offset + grr::size(*current_context, fields.back().id);
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
			throw new std::invalid_argument("type already exists");
		}

		current_context.add(type.id, std::move(type_context{ type.id, type.size, type.name, type.name, type.fields }));
	}

	inline void add_type(context& current_context, const type_declaration& type, type_id base_type)
	{
		if (current_context.contains(type.id)) {
			throw new std::invalid_argument("type already exists");
		}

		current_context.add(type.id, std::move(type_context{ base_type, type.size, type.name, type.name, type.fields }));
	}

	template<typename BaseType>
	constexpr void add_type(context& current_context, const type_declaration& type)
	{
		if (current_context.contains(type.id)) {
			throw new std::invalid_argument("type already exists");
		}

		current_context.add(type.id, std::move(type_context{ obtain_id<BaseType>(), type.size, type.name, type.name, type.fields }));
	}

	template<typename T> 
	void add_type(context& current_context)
	{
		using ClearType = std::remove_pointer_t<std::remove_cv_t<T>>;
		type_declaration new_type = { current_context, grr::type_name<ClearType>() };
		constexpr bool is_aggregate = std::is_aggregate<ClearType>();

#ifdef GRR_PREDECLARE_FIELDS
		if constexpr (is_aggregate) {
			constexpr bool is_visitable = visit_struct::traits::is_visitable<ClearType>::value;
			constexpr bool is_reflectable = pfr::is_implicitly_reflectable_v<ClearType, ClearType>;
			static_assert(is_visitable || is_reflectable, "GRR supports only aggregate types (such as PODs)");

			const ClearType val = {};
			if constexpr (is_visitable) {
				visit_struct::for_each(val, [&val, &new_type](const char* name, const auto& field) {
					const std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(&field) - reinterpret_cast<std::ptrdiff_t>(&val);
					new_type.emplace<std::remove_reference_t<decltype(field)>>(name, offset);
					new_type.size += sizeof(std::remove_reference_t<decltype(field)>);
				});
			} else if constexpr (is_reflectable) {
				pfr::for_each_field(val, [&val, &new_type](const auto& field) {
					const std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(&field) - reinterpret_cast<std::ptrdiff_t>(&val);

					grr::string field_name;
					field_name.resize(14);
					std::snprintf(field_name.data(), 14, "var%u", static_cast<std::uint32_t>(offset));

					new_type.emplace<std::remove_reference_t<decltype(field)>>(field_name.data(), offset);
					new_type.size += sizeof(std::remove_reference_t<decltype(field)>);
				});
			}
		} else {
			if constexpr (!std::is_same_v<ClearType, void>) {
				new_type.size = sizeof(ClearType);
			}
		}
#else
		new_type.size = sizeof(ClearType);
#endif

		new_type.aggregate = is_aggregate;
		grr::add_type(current_context, new_type);
		if constexpr (!std::is_void_v<ClearType>) {
			grr::add_type<ClearType>(current_context, { current_context, grr::type_name<volatile ClearType>(), sizeof(ClearType) });
			grr::add_type<ClearType>(current_context, { current_context, grr::type_name<const ClearType>(), sizeof(ClearType) });
			grr::add_type<ClearType>(current_context, { current_context, grr::type_name<ClearType&>(), sizeof(ClearType) });
			grr::add_type<ClearType>(current_context, { current_context, grr::type_name<volatile ClearType&>(), sizeof(ClearType) });
			grr::add_type<ClearType>(current_context, { current_context, grr::type_name<const ClearType&>(), sizeof(ClearType) });		
			
			grr::add_type<ClearType>(current_context, { current_context, grr::type_name<volatile ClearType const>(), sizeof(ClearType) });
		}

		grr::add_type<ClearType>(current_context, { current_context, grr::type_name<ClearType*>(), sizeof(ClearType*) });	
		grr::add_type<ClearType>(current_context, { current_context, grr::type_name<volatile ClearType*>(), sizeof(ClearType*) });			
		grr::add_type<ClearType>(current_context, { current_context, grr::type_name<const ClearType*>(), sizeof(ClearType*) });		
		
		grr::add_type<ClearType>(current_context, { current_context, grr::type_name<ClearType* const>(), sizeof(ClearType*) });	
		grr::add_type<ClearType>(current_context, { current_context, grr::type_name<volatile ClearType* const>(), sizeof(ClearType*) });
		grr::add_type<ClearType>(current_context, { current_context, grr::type_name<const ClearType* const>(), sizeof(ClearType*) });
	}

	namespace detail
	{
		template<typename... Types>
		void add_types(context& current_context)
		{
			(grr::add_type<Types>(current_context), ...);
		}
	}

	inline context make_context()
	{
		context out_context;
		detail::add_types<GRR_TYPES>(out_context);
		return out_context;
	}
}

#endif