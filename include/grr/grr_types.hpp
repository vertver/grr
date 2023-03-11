/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_TYPES_HPP_INCLUDED
#define GRR_TYPES_HPP_INCLUDED

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
}

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

#endif