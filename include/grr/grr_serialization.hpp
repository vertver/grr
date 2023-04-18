/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_SERIALIZATION_HPP_INCLUDED
#define GRR_SERIALIZATION_HPP_INCLUDED

namespace grr
{
	template<typename T>
	inline string numeric_to_string(T value)
	{
		string string_value;
		string_value.resize(32);

		if constexpr (std::is_same_v<T, std::uint8_t>) {
			snprintf(string_value.data(), 32, "%u", value);
		} else if constexpr (std::is_same_v<T, std::uint16_t>) {
			snprintf(string_value.data(), 32, "%u", value);
		} else if constexpr (std::is_same_v<T, std::uint32_t>) {
			snprintf(string_value.data(), 32, "%u", value);
		} else if constexpr (std::is_same_v<T, std::uint64_t>) {
			snprintf(string_value.data(), 32, "%llu", value);
		} else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
			snprintf(string_value.data(), 32, "%f", value);
		} else if constexpr (std::is_same_v<T, std::int8_t>) {
			snprintf(string_value.data(), 32, "%d", value);
		} else if constexpr (std::is_same_v<T, std::int16_t>) {
			snprintf(string_value.data(), 32, "%d", value);
		} else if constexpr (std::is_same_v<T, std::int32_t>) {
			snprintf(string_value.data(), 32, "%d", value);
		} else if constexpr (std::is_same_v<T, std::int64_t>) {
			snprintf(string_value.data(), 32, "%ll", value);
		} else {
			snprintf(string_value.data(), 32, "%lld", value);
		}

		string_value.shrink_to_fit();
		return string_value;
	}

	inline std::uint64_t stoull(const char* str, int base = 10)
	{
		char* end_ptr;
		return std::strtoull(str, &end_ptr, base);
	}

	inline int64_t stoll(const char* str, int base = 10)
	{
		char* end_ptr;
		return std::strtoll(str, &end_ptr, base);
	}

	inline std::uint32_t stoul(const char* str)
	{
		return std::uint32_t(stoull(str));
	}

	inline double stof(const char* str)
	{
		char* end_ptr;
		return std::strtod(str, &end_ptr);
	}

	inline float stod(const char* str)
	{
		return float(stof(str));
	}

	struct custom_type
	{
		int hello;
		int world;
	};

	namespace detail
	{ 
		template<typename T>
		grr::string stringify_item(T&& value)
		{
			using ValueClearType = grr::clear_type<decltype(value)>;

			if constexpr (std::is_same_v<ValueClearType, bool>) {
				return grr::string(value == true ? "true" : "false");
			} else if constexpr (std::is_same_v<ValueClearType, grr::string>) {
				return value;
			} else if constexpr (std::is_same_v<ValueClearType, grr::string_view>) {
				return value.data();
			} else if constexpr (std::is_enum_v<ValueClearType>) {
				return grr::numeric_to_string(static_cast<int64_t>(value));
			} else if constexpr (std::is_integral_v<ValueClearType>) {
				return grr::numeric_to_string(static_cast<ValueClearType>(value));
			} else {
				return "";
				//return grr::to_string<ValueClearType>(value);
			}
		}

		template<typename T>
		T unstringify_item(grr::string&& value)
		{
			return T();
		}
	}

	template<typename T>
	struct custom_serializer
	{
		static constexpr bool exists = false;
	};

	template<>
	struct custom_serializer<custom_type>
	{
		static constexpr bool exists = true;

		static inline grr::type_id id()
		{
			return grr::obtain_id("custom_type");
		}

		static inline bool verify_id(grr::type_id cmp_id)
		{
			return (id() == cmp_id);
		}

		static inline grr::string stringify(custom_type&& value)
		{
			return numeric_to_string(value.hello) + " " + numeric_to_string(value.world);
		}

		static inline custom_type unstrigify(grr::string&& value)
		{
			return {};
		}

		static inline std::size_t serialize_size(custom_type&& value)
		{
			return sizeof(custom_type);
		}

		static inline void serialize(custom_type&& value, void* memory)
		{
			*reinterpret_cast<custom_type*>(memory) = value;
		}

		static inline custom_type deserialize(const void* memory)
		{
			return (*reinterpret_cast<const custom_type*>(memory));
		}
	};

	template<typename T>
	grr::string stringify(T&& value)
	{
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		static_assert(!std::is_pointer_v<ClearType> || std::is_same_v<ClearType, char*>, "Pointers are not supported by grr::stringify.");

		grr::string string_value;  
		if constexpr (custom_serializer<ClearType>::exists) {
			return custom_serializer<ClearType>::stringify(value);
		} else {
			if constexpr (std::is_same_v<ClearType, char*>) {
				return grr::string(value);
			} else if constexpr (grr::type_exists<ClearType::value_type>::value) {
				grr::string vector_string;
				vector_string += "{ ";
				if constexpr (grr::type_exists<ClearType::key_type>::value && grr::is_key_value_map_v<ClearType>) {
					for (const auto& [key, val] : value) {
						vector_string += "(";
						vector_string += grr::stringify(key);
						vector_string += " ";
						vector_string += grr::stringify(val);
						vector_string += ")";
						vector_string += " ";
					}
				} else {
					for (const auto& elem : value) {
						vector_string += grr::stringify(elem);
						vector_string += " ";
					}
				}

				vector_string += "}";	
				return vector_string;
			} else {
				if constexpr (custom_serializer<ClearType>::exists) {
					return custom_serializer<ClearType>::stringify(value);
				} else {
					return grr::detail::stringify_item(value);
				}
			}
		}
	}

	template<typename T>
	GRR_CONSTEXPR T unstringify(grr::string_view&& value)
	{
		using ClearType = std::remove_reference_t<std::remove_cv_t<T>>;
		static_assert(!std::is_pointer_v<ClearType> || std::is_same_v<ClearType, char*>, "Pointers are not supported by grr::unstringify.");

		if constexpr (std::is_same_v<ClearType, char*>) {
			return value.data();
		} else if constexpr (grr::type_exists<ClearType::value_type>::value) {
			grr::string_view element_string;
			size_t offset = value.find_first_of('{');
			if (offset == size_t(-1)) {
				// #TODO: parsing error
				return {};
			}

			offset = value.find_first_not_of(' ', offset + 1);
			if (offset == size_t(-1)) {
				// #TODO: parsing error
				return {};
			}

			const size_t offset_end = value.find_last_of('}');
			while (offset != size_t(-1) && offset < offset_end) {
				constexpr bool is_key_value_map = grr::is_key_value_map_v<ClearType>;
		
				size_t begin_offset = value.find_first_not_of(is_key_value_map ? '(' : ' ', offset);
				size_t end_offset = value.find_first_of(is_key_value_map ? ')' : ' ', begin_offset);
				if (end_offset == size_t(-1)) {
					end_offset = offset_end;
				}

				if (end_offset == begin_offset) {
					break;
				}

				offset = end_offset;
				element_string = grr::string_view(value.data() + begin_offset, value.data() + end_offset);
				if constexpr (grr::type_exists<ClearType::key_type>::value) {
					if constexpr (is_key_value_map) {
						// this is std::map or equal
					} else {
						// this is std::set or equal
					}
				}
			}
		} else {
			
		}
	}
}

#endif