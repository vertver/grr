/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_BASE_EXC_HPP_INCLUDED
#define GRR_BASE_EXC_HPP_INCLUDED

namespace grr
{
	inline void rename(context& current_context, type_id id, const char* new_name)
	{
		std::error_code err;
		rename(current_context, id, new_name);
		if (err) {
			return;
		}
	}

	inline void rename(context& current_context, type_id id, const string_view& new_name)
	{
		std::error_code err;
		rename(current_context, id, new_name);
		if (err) {
			return;
		}
	}

	inline void rename(context& current_context, type_id id, std::size_t field_idx, const char* new_name)
	{
		std::error_code err;
		rename(current_context, id, field_idx, new_name);
		if (err) {
			return;
		}
	}

	inline void rename(context& current_context, type_id id, std::size_t field_idx, const string_view& new_name)
	{
		std::error_code err;
		rename(current_context, id, field_idx, new_name);
		if (err) {
			return;
		}
	}

	template<typename T>
	inline void rename(context& current_context, type_id id, const char* new_name)
	{
		std::error_code err;
		rename<T>(current_context, new_name);
		if (err) {
			return;
		}
	}

	template<typename T>
	inline void rename(context& current_context, type_id id, const string_view& new_name)
	{
		std::error_code err;
		rename<T>(current_context, new_name);
		if (err) {
			return;
		}
	}

	template<typename T>
	inline void rename(context& current_context, type_id id, std::size_t field_idx, const char* new_name)
	{
		std::error_code err;
		rename<T>(current_context, field_idx, new_name);
		if (err) {
			return;
		}
	}

	template<typename T>
	inline void rename(context& current_context, type_id id, std::size_t field_idx, const string_view& new_name)
	{
		std::error_code err;
		rename<T>(current_context, field_idx, new_name);
		if (err) {
			return;
		}
	}

	inline std::size_t offset(context& current_context, type_id id, std::size_t field_idx)
	{
		std::error_code err;
		offset(current_context, id, field_idx, err);
		if (err) {
			return 0;
		}
	}

	template<typename T>
	inline std::size_t offset(context& current_context, std::size_t field_idx)
	{
		std::error_code err;
		offset<T>(current_context, field_idx, err);
		if (err) {
			return 0;
		}
	}

	template<std::size_t recursion_level = 0>
	static constexpr void visit(const grr::context& context, auto data, type_id id, auto&& func)
	{
		std::error_code err;
		visit<recursion_level>(context, data, id, err, func);
		if (err) {
			return;
		}
	}
	
	template< typename T, std::size_t recursion_levels = 0>
	static constexpr void visit(const grr::context& context, T& data, auto&& func)
	{
		std::error_code err;
		visit<T, recursion_levels>(context, data, err, func);
		if (err) {
			return;
		}
	}

	inline void add_type(context& current_context, const type_declaration& type)
	{	
		std::error_code err;
		add_type(current_context, type, err);
		if (err) {
			return;
		}
	}

	inline void add_type(context& current_context, const type_declaration& type, type_id base_type)
	{	
		std::error_code err;
		add_type(current_context, type, base_type, err);
		if (err) {
			return;
		}
	}

	template<typename BaseType>
	constexpr void add_type(context& current_context, const type_declaration& type)
	{
		std::error_code err;
		add_type<BaseType>(current_context, type, err);
		if (err) {
			return;
		}
	}

	template<typename T> 
	void add_type(context& current_context)
	{
		std::error_code err;
		add_type<T>(current_context, err);
		if (err) {
			return;
		}
	}

	inline context make_context()
	{
		std::error_code err;
		context out_context = make_context(err);
		if (err) {
			return {};
		}

		return out_context;
	}
}

#endif