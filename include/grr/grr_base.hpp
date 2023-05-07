/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_BASE_HPP_INCLUDED
#define GRR_BASE_HPP_INCLUDED
#include <grr/detail/name_parser.hpp>

#ifndef GRR_RETURN_IF_FAILED
#define GRR_RETURN_IF_FAILED(x) \
	(x); \
	if (err) { \
		return;\
	}
#endif

namespace grr
{
	enum class errors : int
	{
		invalid_argument,
		unregistered_id,
		already_registered,
		parsing_failed,
		out_of_range
	};

	struct error_category : public std::error_category
	{
		std::string message(int c) const
		{
			static const char* err_msg[] =
			{
				"Invalid argument",
				"Unregisted ID",
				"Already registered",
				"Parsing failed",
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
		GRR_CONSTEXPR field(const char* new_name, type_id new_id, std::size_t new_offset)
			: name(new_name), id(new_id), offset(new_offset) {}

		GRR_CONSTEXPR field(const string_view& new_name, type_id new_id, std::size_t new_offset)
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

		void rename(type_id id, const char* new_name, std::error_code& err)
		{
			const auto& it = storage.find(id);
			if (it == storage.end()) {
				err = make_error_code(errors::unregistered_id);
				return;
			}

			it->second.name = new_name;
		}

		void rename(type_id id, const string_view& new_name, std::error_code& err)
		{
			const auto& it = storage.find(id);
			if (it == storage.end()) {
				err = make_error_code(errors::unregistered_id);
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
	constexpr auto type_name()
	{
		return grr::detail::compiler_type_name<T>(0);
	}

	inline const char* type_name(const context& current_context, type_id id)
	{
		if (!current_context.contains(id)) {
			return "";
		}

		return current_context.at(id).name.c_str();
	}

	template<typename T>
	constexpr T binhash(const std::string_view& str)
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

	template<typename T, std::size_t max_indexes = 64>
	constexpr 
	T
	serializable_hash(const grr::string_view& str)
	{
		constexpr grr::string_view strings[] =
		{
			"struct",       // MSVC stuff
			"class",        // MSVC stuff
			"__cxx11::",    // GCC stuff
			"__cxx14::",    // GCC stuff
			"__cxx17::",    // GCC stuff
			"__cxx20::",    // GCC stuff
			"__cxx23::",    // GCC stuff
			" "             // MSVC and GCC stuff
		};

		T hash = T(5381);
		std::size_t indexes_count = 0;
		std::array<std::size_t, max_indexes> indexes;
		for (const grr::string_view& comparable : strings) {
			std::size_t start = static_cast<std::size_t>(-1);
			for (size_t i = 0; i < str.size(); i++) {
				if (start == std::size_t(-1)) {
					if (str.at(i) == comparable.at(0)) {
						start = i;
						continue;
					}
				} else {
					const size_t offset = i - start;
					if (offset >= comparable.size()) {
						indexes[indexes_count] = start;
						indexes[indexes_count + 1] = start + comparable.size();
						indexes_count += 2;
						if (indexes_count == indexes.size()) {
							break;
						}

						start = std::size_t(-1);
						continue;
					}

					if (str.at(i) != comparable.at(offset)) {
						start = std::size_t(-1);
						continue;
					}
				}
			}
		}
    
		std::size_t current_idx = 0;
		std::sort(indexes.begin(), indexes.begin() + indexes_count);
		for (size_t i = 0; i < str.size(); i++) {
			if (current_idx > 0) {
				if (current_idx >= indexes_count) {
					break;
				}

				if (i >= indexes[current_idx + 1]) {
					current_idx += 2;
				}

				if (current_idx >= indexes_count) {
					break;
				}

				if (i >= indexes[current_idx] && i < indexes[current_idx + 1]) {
					continue;
				}
			}

			hash *= 0x21;
			hash += str[i];
		}

		return hash;
	}

	constexpr type_id obtain_id(const char* name)
	{
		return serializable_hash<type_id>(name);
	}	
	
	constexpr type_id obtain_id(const grr::string_view& name)
	{
		return serializable_hash<type_id>(name);
	}

	template<typename T>
	constexpr type_id obtain_id()
	{
		return obtain_id(type_name<grr::clean_type<T>>());
	}

	inline std::size_t size(const context& current_context, type_id id)
	{
		return current_context.size(id);
	}

	inline bool contains(const context& current_context, type_id id)
	{
		return current_context.contains(id);
	}

	inline void rename(context& current_context, type_id id, std::size_t field_idx, const char* new_name, std::error_code& err)
	{
		if (!current_context.contains(id)) {
			err = make_error_code(errors::unregistered_id);
			return;
		}

		auto& fields = current_context.at(id).fields;
		if (field_idx >= fields.size()) {
			err = make_error_code(errors::invalid_argument);
			return;
		}

		fields.at(field_idx).name = new_name;
	}

	inline void rename(context& current_context, type_id id, std::size_t field_idx, const string_view& new_name, std::error_code& err)
	{
		if (!current_context.contains(id)) {
			err = make_error_code(errors::unregistered_id);
			return;
		}

		auto& fields = current_context.at(id).fields;
		if (field_idx >= fields.size()) {
			err = make_error_code(errors::invalid_argument);
			return;
		}

		fields.at(field_idx).name = string(new_name.begin(), new_name.end());
	}

	inline void rename(context& current_context, type_id id, const char* new_name, std::error_code& err)
	{
		current_context.rename(id, new_name, err);
	}
	
	inline void rename(context& current_context, type_id id, const string_view& new_name, std::error_code& err)
	{
		current_context.rename(id, new_name, err);
	}

	inline std::size_t offset(context& current_context, type_id id, std::size_t field_idx, std::error_code& err)
	{
		if (!current_context.contains(id)) {
			err = make_error_code(errors::unregistered_id);
			return 0;
		}

		auto& fields = current_context.at(id).fields;
		if (field_idx >= fields.size()) {
			err = make_error_code(errors::invalid_argument);
			return 0;
		}

		return fields.at(field_idx).offset;
	}

	inline type_id base_type(context& current_context, type_id id)
	{
		return current_context.at(id).base_type;
	}

	template<typename T>
	constexpr type_id base_type()
	{
		return obtain_id<std::remove_pointer_t<grr::clean_type<T>>>();
	}

	template<typename T>
	inline void rename(context& current_context, std::size_t field_idx, const string_view& new_name, std::error_code& err)
	{
		constexpr type_id id = obtain_id<grr::clean_type<T>>();
		rename(current_context, id, field_idx, new_name, err);
	}	
	
	template<typename T>
	inline void rename(context& current_context, std::size_t field_idx, const char* new_name, std::error_code& err)
	{
		constexpr type_id id = obtain_id<grr::clean_type<T>>();
		rename(current_context, id, field_idx, new_name, err);
	}

	template<typename T>
	inline void rename(context& current_context, const string_view& new_name, std::error_code& err)
	{
		current_context.rename(obtain_id<grr::clean_type<T>>(), new_name, err);
	}
	 
	template<typename T>
	inline void rename(context& current_context, const char* new_name, std::error_code& err)
	{
		current_context.rename(obtain_id<grr::clean_type<T>>(), new_name, err);
	}

	template<typename T>
	inline bool contains(const context& current_context)
	{
		return current_context.contains(obtain_id<grr::clean_type<T>>());
	}

	template<typename T>
	inline bool size(const context& current_context)
	{
		return current_context.size(obtain_id<grr::clean_type<T>>());
	}

	template<typename T>
	inline std::size_t offset(context& current_context, std::size_t field_idx, std::error_code& err)
	{
		constexpr type_id id = obtain_id<grr::clean_type<T>>();
		return offset(current_context, id, field_idx, err);
	}

	namespace detail
	{
		template<typename T>
		static constexpr void visit_static_once(auto data, const char* name, type_id id, bool& called, auto&& func)
		{
			using CleanDataType = std::remove_pointer_t<decltype(data)>;

			if constexpr (!std::is_same_v<T, void>) {
				constexpr type_id current_id = grr::obtain_id<T>();
				if (!called && current_id == id) {
					if constexpr (std::is_const_v<CleanDataType>) {
						func(*reinterpret_cast<const T*>(data), name);
					}
					else {
						func(*reinterpret_cast<T*>(data), name);
					}

					called = true;
				}
			} 
	
			constexpr type_id current_ptr_id = grr::obtain_id<T*>();
			if (!called && current_ptr_id == id) {
				if constexpr (std::is_const_v<CleanDataType>) {
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

		template<std::size_t recursion_level = 0>
		static inline bool visit(const grr::context& context, auto data, type_id id, auto&& func)
		{		
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
						return false;
					}

					const auto& field_type = context.obtain(field_id);
					func(std::make_pair(field_type.size, std::make_pair(field_ptr, field_id)), cfield.name.data());
				}
			}

			return true;
		}
	}

	template<std::size_t recursion_level = 0>
	static inline void visit(const grr::context& context, auto data, type_id id, std::error_code& err, auto&& func)
	{
		if (!context.contains(id)) {
			err = make_error_code(errors::unregistered_id);
			return;
		}

		if (!grr::detail::visit<recursion_level>(context, data, id, func)) {
			err = make_error_code(errors::unregistered_id);
			return;
		}
	}

	template<typename T, std::size_t recursion_level = 0>
	static inline void visit(const grr::context& context, T& data, std::error_code& err, auto&& func)
	{
		constexpr type_id id = grr::obtain_id<T>();
		if constexpr (std::is_const_v<T>) {
			grr::visit<recursion_level>(context, reinterpret_cast<const void*>(&data), id, err, func);
		} else {
			grr::visit<recursion_level>(context, reinterpret_cast<void*>(&data), id, err, func);
		}
	}
	
	struct type_declaration
	{
		bool aggregate = false;
		const context* current_context;
		std::size_t size;
		string name;
		type_id id;
		vector<field> fields;

		GRR_CONSTEXPR type_declaration() = delete;
		GRR_CONSTEXPR type_declaration(type_declaration&) = delete;
		GRR_CONSTEXPR type_declaration(const type_declaration&) = delete;
		GRR_CONSTEXPR type_declaration(type_declaration&&) = default;

		GRR_CONSTEXPR type_declaration(const context& in_context, const char* type_name) noexcept
			: current_context(&in_context), name(type_name), id(obtain_id(type_name)), size(0) {}
		
		GRR_CONSTEXPR type_declaration(const context& in_context, const string_view& type_name) noexcept
			: current_context(&in_context), name(type_name.begin(), type_name.end()), id(obtain_id(type_name)), size(0) {}

		GRR_CONSTEXPR type_declaration(const context& in_context, type_id in_id, const char* type_name) noexcept
			: current_context(&in_context), name(type_name), id(in_id), size(0) {}

		GRR_CONSTEXPR type_declaration(const context& in_context, type_id in_id, const string_view& type_name) noexcept
			: current_context(&in_context), name(type_name.begin(), type_name.end()), id(in_id), size(0) {}
		
		GRR_CONSTEXPR type_declaration(const context& in_context, const char* type_name, std::size_t new_size) noexcept
			: current_context(&in_context), name(type_name), id(obtain_id(type_name)), size(new_size) {}
		
		GRR_CONSTEXPR type_declaration(const context& in_context, const string_view& type_name, std::size_t new_size) noexcept
			: current_context(&in_context), name(type_name.begin(), type_name.end()), id(obtain_id(type_name)), size(new_size) {}

		bool field_erase(const char* field_name)
		{
			const auto field_hash = binhash<std::uint32_t>(field_name);
			for (auto it = fields.begin(); it != fields.end(); it++) {
				const auto& field_elem = *it;
				if (field_hash == binhash<std::uint32_t>(field_elem.name.data())) {
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
		void emplace(const char* field_name, std::error_code& err)
		{
			constexpr type_id current_id = obtain_id<T>();
			if (!grr::contains(*current_context, current_id)) {
				err = make_error_code(errors::unregistered_id);
				return;
			}

			const std::size_t offset = fields.empty() ? 0 : fields.back().offset + grr::size(*current_context, fields.back().id);
			fields.emplace_back(std::move(field(field_name, current_id, offset)));
		}

		template<typename T>
		void emplace(const char* field_name, std::size_t offset, std::error_code& err)
		{
			constexpr type_id current_id = obtain_id<T>();
			if (!grr::contains(*current_context, current_id)) {
				err = make_error_code(errors::unregistered_id);
				return;
			}

			fields.emplace_back(std::move(field(field_name, current_id, offset)));
		}

		void erase(std::size_t idx, std::error_code& err)
		{
			if (!field_erase(idx)) {
				err = make_error_code(errors::invalid_argument);
				return;
			}
		}

		void erase(const char* field_name, std::error_code& err)
		{
			if (!field_erase(field_name)) {
				err = make_error_code(errors::invalid_argument);
				return;
			}
		}
	};

	inline void add_type(context& current_context, const type_declaration& type, std::error_code& err)
	{
		if (current_context.contains(type.id)) {
			err = make_error_code(errors::already_registered);
			return;
		}

		current_context.add(type.id, std::move(type_context{ type.id, type.size, type.name, type.name, type.fields }));
	}

	inline void add_type(context& current_context, const type_declaration& type, type_id base_type, std::error_code& err)
	{
		if (current_context.contains(type.id)) {
			err = make_error_code(errors::already_registered);
			return;
		}

		current_context.add(type.id, std::move(type_context{ base_type, type.size, type.name, type.name, type.fields }));
	}

	template<typename BaseType>
	constexpr void add_type(context& current_context, const type_declaration& type, std::error_code& err)
	{
		if (current_context.contains(type.id)) {
			err = make_error_code(errors::already_registered);
			return;
		}

		current_context.add(type.id, std::move(type_context{ obtain_id<BaseType>(), type.size, type.name, type.name, type.fields }));
	}

	template<typename T> 
	void add_type(context& current_context, std::error_code& err)
	{
		using CleanType = grr::clean_type<T>;
		type_declaration new_type = type_declaration(current_context, grr::obtain_id<CleanType>(), grr::type_name<CleanType>());
		constexpr bool is_aggregate = std::is_aggregate<CleanType>();

#ifdef GRR_PREDECLARE_FIELDS
		if constexpr (is_aggregate) {
			constexpr bool is_visitable = visit_struct::traits::is_visitable<CleanType>::value;
			constexpr bool is_reflectable = pfr::is_implicitly_reflectable_v<CleanType, CleanType>;
			static_assert(grr::is_reflectable_v<T>, "GRR supports only aggregate types (such as PODs)");

			const CleanType val = {};
			if constexpr (is_visitable) {
				visit_struct::for_each(val, [&err, &val, &new_type](const char* name, const auto& field) {
					const std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(&field) - reinterpret_cast<std::ptrdiff_t>(&val);
					new_type.emplace<std::remove_reference_t<decltype(field)>>(name, offset, err);
					if (err) {
						return;
					}

					new_type.size += sizeof(std::remove_reference_t<decltype(field)>);
				});
			} else if constexpr (is_reflectable) {
				pfr::for_each_field(val, [&err, &val, &new_type](const auto& field) {
					const std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(&field) - reinterpret_cast<std::ptrdiff_t>(&val);

					grr::string field_name;
					field_name.resize(14);
					std::snprintf(field_name.data(), 14, "var%u", static_cast<std::uint32_t>(offset));

					new_type.emplace<std::remove_reference_t<decltype(field)>>(field_name.data(), offset, err);
					new_type.size += sizeof(std::remove_reference_t<decltype(field)>);
				});
			}
		} else {
			if constexpr (!std::is_same_v<CleanType, void>) {
				new_type.size = sizeof(CleanType);
			}
		}
#else
		new_type.size = sizeof(CleanType);
#endif

		new_type.aggregate = is_aggregate;
		grr::add_type(current_context, new_type, err);
		if constexpr (!std::is_void_v<CleanType>) {
			GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<volatile CleanType>(), sizeof(CleanType) }, err));
			GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<const CleanType>(), sizeof(CleanType) }, err));
			GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<CleanType&>(), sizeof(CleanType) }, err));
			GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<volatile CleanType&>(), sizeof(CleanType) }, err));
			GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<const CleanType&>(), sizeof(CleanType) }, err));
			GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<volatile CleanType const>(), sizeof(CleanType) }, err));
		}

		GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<CleanType*>(), sizeof(CleanType*) }, err));
		GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<volatile CleanType*>(), sizeof(CleanType*) }, err));
		GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<const CleanType*>(), sizeof(CleanType*) }, err));
		GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<CleanType* const>(), sizeof(CleanType*) }, err));
		GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<volatile CleanType* const>(), sizeof(CleanType*) }, err));
		GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(current_context, { current_context, grr::type_name<const CleanType* const>(), sizeof(CleanType*) }, err));
	}

	namespace detail
	{
		template<typename... Types>
		void add_types(context& current_context, std::error_code& err)
		{
			(grr::add_type<Types>(current_context, err), ...);
		}
	}

	inline context make_context(std::error_code& err)
	{
		context out_context;
		detail::add_types<GRR_TYPES>(out_context, err);
		return out_context;
	}
}

#undef GRR_RETURN_IF_FAILED
#endif