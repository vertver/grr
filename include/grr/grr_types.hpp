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
	using allocator = std::allocator<T>;

	template<typename T>
	using vector = GRR_VECTOR<T>;

	template<typename K, typename V>
	using hash_map = GRR_HASH_MAP<K, V>;

	template<typename K>
	using hash_set = GRR_HASH_SET<K>;

#ifdef _MSC_VER
	using wstring = GRR_WIDESTRING;
	using wstring_view = GRR_WIDESTRING_VIEW;
#endif

	using string = GRR_STRING;
	using string_view = GRR_STRING_VIEW;
	using type_id = GRR_TYPE_ID;

	using ptr_pair = std::pair<std::uint64_t, std::pair<void*, type_id>>;
	using const_ptr_pair = std::pair<std::uint64_t, std::pair<const void*, type_id>>;
}

#ifndef GRR_MSVC_TYPES
#ifdef _MSC_VER
#define GRR_MSVC_TYPES \
	wchar_t, \
	__int32, \
	unsigned __int32, \
	grr::wstring, \
	grr::wstring_view, 
#else
#define GRR_MSVC_TYPES 
#endif
#endif

#ifndef GRR_CLANG_TYPES
#ifdef __clang__
#define GRR_CLANG_TYPES 
#else
#define GRR_CLANG_TYPES 
#endif
#endif

#ifndef GRR_GCC_TYPES
#ifdef __GNUC__
#define GRR_GCC_TYPES 
#else
#define GRR_GCC_TYPES 
#endif
#endif

#ifndef GRR_TYPES
#define GRR_TYPES \
	void, \
	char, \
	short, \
	long, \
	long long, \
	unsigned char, \
	unsigned short, \
	unsigned long, \
	unsigned long long, \
	GRR_MSVC_TYPES \
	GRR_CLANG_TYPES \
	GRR_GCC_TYPES \
	\
	float, \
	double, \
	\
	grr::ptr_pair, \
	grr::const_ptr_pair, \
	grr::string, \
	grr::string_view \
	GRR_USER_TYPES
#endif

namespace grr
{
	template<typename T>
	struct is_statically_reflectable : std::false_type {};

	template<typename T>
	constexpr bool is_statically_reflectable_v = is_fallback_type<T>::value;

	template<typename T>
	struct is_fallback_type : std::false_type {};
	template<>
	struct is_fallback_type<ptr_pair> : std::true_type {};
	template<>
	struct is_fallback_type<const_ptr_pair> : std::true_type {};

	template<typename T>
	constexpr bool is_fallback_type_v = is_fallback_type<T>::value;

	template<typename T>
	constexpr bool is_reflectable_v = 
		visit_struct::traits::is_visitable<
			std::remove_pointer_t<std::remove_cv_t<T>>
		>::value ||
		pfr::is_implicitly_reflectable<
			std::remove_pointer_t<std::remove_cv_t<T>>, 
			std::remove_pointer_t<std::remove_cv_t<T>>
		>::value;

	template<class T> struct EmptyType { typedef void type; };

	template<class T, class U = void>
	struct type_exists : std::false_type {};

	template<class T>
	struct type_exists<T, typename EmptyType<T>::type> : std::true_type {};

	template<class T>
	using clear_type = std::remove_reference_t<std::remove_cv_t<T>>;

	template<typename T, typename ClearType = grr::clear_type<T>>
	constexpr bool is_key_value_map_v = grr::type_exists<ClearType::key_type>::value && grr::type_exists<ClearType::mapped_type>::value;
}

#endif